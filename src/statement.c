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
 \file     statement.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     5/1/2002

 \par
 Statements are used to indicate flow of execution for a given always or initial
 block.  Each statement is assigned to exactly one expression tree and it
 contains a pointer to the next statement if its expression tree evaluates to
 TRUE (non-zero value) or FALSE (zero value).

 \par
 The head bit indicates that this statement should be loaded into the
 pre-simulation statement queue if any of its expressions change value in the
 current timestep.  To begin with, the first statement in the always/initial block
 has this bit set, all other statements have this bit cleared.

 \par
 The stop bits are used for CDD output.  If a statement has these bits set, it will
 not traverse its next_true or next_false, respectively, paths when outputting.  This is necessary
 when statement paths merge back to the same path or when statement loops are encountered.
 If this bit was not used, the statements output after the merge/loop would be output more than
 once.  Consider the following Verilog code snippet:

 \code
 intial begin
        a = 0;
        if( a )
          b = 1;
        else
          b = 0;
        c = a | b;
 end
 \endcode

 \par
 In this example there are five statements.  They are the following:

 \par
 -# \code a = 0; \endcode
 -# \code if( a ) \endcode
 -# \code b = 1; \endcode
 -# \code b = 0; \endcode
 -# \code c = a | b; \endcode

 \par
 Notice that the if statement has two paths.  If a is TRUE, statement 3 is executed;
 otherwise, statement 4 is executed.  Both statements 3 and 4 then merge to execute
 statement 5.  If the entire statement tree were printed without a STOP bit, you can
 see that statement 5 would be output twice; once for the TRUE branch and once for
 the FALSE branch.

 \par
 To eliminate this redundancy, the stop_true and stop_false bits are set
 on statement 4.  The last statement of a TRUE path before merging always gets its
 stop bit set.  Additionally, once a statement gets its stop bit set by
 the parser, this value must be maintained (never cleared).

 \par Cyclic Statement Trees
 Many times a statement tree will "loopback" on itself.  These statement trees are
 considered to be cyclic in nature and, as such, must have leaf statements of the
 statement tree tied to the first statement of the tree.  The following Verilog
 constructs have cyclic behavior:  \c always, \c forever, \c for, \c repeat, and
 \c while.

 \par Traversing Statement Tree
 Starting at the head statement, the value of the head statement is determined to be
 TRUE or FALSE.  If the value is TRUE (non-zero), the next_true path is taken.  If
 the value is FALSE (zero), the next_false path is taken.  For statements that
 express an assignment (or other statements that do not branch), both the next_true
 and next_false paths point to the same statement.  For statements that express a
 wait-for-event (ex. \c wait, \c @, etc.), the next_true pointer will point to the
 next statement in the tree; however, the next_false pointer will point to NULL.
 This NULL assignment indicates to the statement simulation engine that this statement
 will receive the head bit and the current statement tree is finished for this
 timestep.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "statement.h"
#include "expr.h"
#include "util.h"
#include "link.h"
#include "sim.h"
#include "db.h"
#include "iter.h"
#include "stmt_blk.h"


extern char     user_msg[USER_MSG_LENGTH];
extern exp_info exp_op_info[EXP_OP_NUM];

/*!
 Pointer to head of statement loop list.
*/
static stmt_loop_link* stmt_loop_head = NULL;

/*!
 Pointer to tail of statement loop list.
*/
static stmt_loop_link* stmt_loop_tail = NULL;


/*!
 \return Returns pointer to the newly created statement.

 Creates a new statement structure from heap memory and initializes it with the
 specified parameter information.
*/
statement* statement_create(
  expression* exp,   /*!< Pointer to root expression of expression tree for this statement */
  func_unit*  funit  /*!< Pointer to functional unit that this statement exists in */
) { PROFILE(STATEMENT_CREATE);

  statement* stmt;  /* Pointer to newly created statement */

  stmt                    = (statement*)malloc_safe( sizeof( statement ) );
  stmt->exp               = exp;
  stmt->exp->parent->stmt = stmt;
  stmt->next_true         = NULL;
  stmt->next_false        = NULL;
  stmt->conn_id           = 0;
  stmt->suppl.all         = 0;
  stmt->funit             = funit;

  PROFILE_END;

  return( stmt );

}

/*!
 Displays the current contents of the statement loop list for debug purposes only.
*/
void statement_queue_display() {

  stmt_loop_link* sll;  /* Pointer to current statement loop link */

  printf( "Statement loop list:\n" );

  sll = stmt_loop_head;
  while( sll != NULL ) {
    printf( "  id: %d, next_true: %d, stmt: %s  ", sll->id, sll->next_true, expression_string( sll->stmt->exp ) );
    if( sll == stmt_loop_head ) {
      printf( "H" );
    }
    if( sll == stmt_loop_tail ) {
      printf( "T" );
    }
    printf( "\n" );
    sll = sll->next;
  }

}

/*!
 \param stmt       Pointer of statement waiting to be linked.
 \param id         ID of statement to be read out later.
 \param next_true  Set to TRUE if the specified ID is for the next_true statement.

 Creates a new statement loop link for the specified parameters and adds this
 element to the top of the statement loop queue.
*/
static void statement_queue_add(
  statement* stmt,
  int        id,
  bool       next_true
) { PROFILE(STATEMENT_QUEUE_ADD);

  stmt_loop_link* sll;  /* Pointer to newly created statement loop link */

  /* Create statement loop link element */
  sll = (stmt_loop_link*)malloc_safe( sizeof( stmt_loop_link ) );

  /* Populate statement loop link with specified parameters */
  sll->stmt      = stmt;
  sll->id        = id;
  sll->next_true = next_true;
  sll->next      = NULL;

  /* Add to top of statement loop queue */
  if( stmt_loop_head == NULL ) {
    stmt_loop_head = stmt_loop_tail = sll;
  } else {
    stmt_loop_tail->next = sll;
    stmt_loop_tail       = sll;
  }

  PROFILE_END;

}

/*!
 \param stmt  Pointer to statement being read out of the CDD.
 
 Compares the specified statement against the top of the statement loop queue.  If
 an ID in the queue matches this statement's ID, the element is removed and the
 next_true and next_false pointers of the stored statement are pointed
 to the specified statement.  The next head is also compared against this statement
 and the process is repeated until a match is not found.
*/
static void statement_queue_compare(
  statement* stmt
) { PROFILE(STATEMENT_QUEUE_COMPARE);

  stmt_loop_link* sll;       /* Pointer to current element in statement loop list */
  stmt_loop_link* tsll;      /* Temporary pointer to current element in statement loop list */
  stmt_loop_link* last_sll;  /* Pointer to last parsed element in statement loop list */

  sll      = stmt_loop_head;
  last_sll = NULL;

  while( sll != NULL ) {

    /* If we have a match */
    if( stmt->exp->id == sll->id ) {

      /* Set next_true and next_false pointers */
      if( (sll->stmt->next_true == NULL) && sll->next_true ) {
        sll->stmt->next_true = stmt;
      }
      if( (sll->stmt->next_false == NULL) && !sll->next_true ) {
        sll->stmt->next_false = stmt;
      }
       
      /* Remove this element from the list */
      if( (stmt_loop_head == sll) && (stmt_loop_tail == sll) ) {
        stmt_loop_head = stmt_loop_tail = NULL;
      } else if( stmt_loop_head == sll ) {
        stmt_loop_head = sll->next;
      } else if( stmt_loop_tail == sll ) {
        stmt_loop_tail       = last_sll;
        stmt_loop_tail->next = NULL;
      } else {
        last_sll->next = sll->next;
      }

      /* Deallocate the current element */
      tsll = sll;
      sll  = sll->next;
      free_safe( tsll, sizeof( stmt_loop_link ) );

    } else {

      last_sll = sll;
      sll      = sll->next;

    }

  }

  PROFILE_END;

}

/*!
 \param stmt   Pointer to statement block to size elements for
 \param funit  Pointer to functional unit containing this statement block

 \throws anonymous expression_resize statement_size_elements statement_size_elements statement_size_elements

 Recursively sizes all elements for the given statement block.
*/
void statement_size_elements(
  statement* stmt,
  func_unit* funit
) { PROFILE(STATEMENT_SIZE_ELEMENTS);

  if( stmt != NULL ) {

    /* Size the current statement */
    expression_resize( stmt->exp, funit, TRUE, FALSE );

    /* Iterate to the next statement */
    if( stmt->next_true == stmt->next_false ) {
      if( stmt->suppl.part.stop_true == 0 ) {
        statement_size_elements( stmt->next_true, funit );
      }
    } else {
      if( stmt->suppl.part.stop_false == 0 ) {
        statement_size_elements( stmt->next_false, funit );
      }
      if( stmt->suppl.part.stop_true == 0 ) {
        statement_size_elements( stmt->next_true, funit );
      }
    }

  }

  PROFILE_END;

}
    
/*!
 Recursively writes the contents of the specified statement tree (and its
 associated expression trees to the specified output stream.
*/
void statement_db_write(
  statement* stmt,       /*!< Pointer to statement to write out value */
  FILE*      ofile,      /*!< Pointer to output file to write statement line to */
  bool       ids_issued  /*!< Specifies that IDs were issued just prior to calling this function */
) { PROFILE(STATEMENT_DB_WRITE);

  assert( stmt != NULL );

  /* Write out contents of this statement last */
  fprintf( ofile, "%d %d %x %d %d",
    DB_TYPE_STATEMENT,
    expression_get_id( stmt->exp, ids_issued ),
    (stmt->suppl.all & 0xff),
    ((stmt->next_true   == NULL) ? 0 : expression_get_id( stmt->next_true->exp, ids_issued )),
    ((stmt->next_false  == NULL) ? 0 : expression_get_id( stmt->next_false->exp, ids_issued ))
  );

  fprintf( ofile, "\n" );

  PROFILE_END;

}

/*!
 \param stmt   Pointer to root of statement tree to output
 \param ofile  Pointer to output file to write statements to

 Traverses specified statement tree, outputting all statements within that tree.
*/
void statement_db_write_tree(
  statement* stmt,
  FILE*      ofile
) { PROFILE(STATEMENT_DB_WRITE_TREE);

  if( stmt != NULL ) {

    /* Traverse down the rest of the statement block */
    if( (stmt->next_true == stmt->next_false) && (stmt->suppl.part.stop_true == 0) ) {
      statement_db_write_tree( stmt->next_true, ofile );
    } else {
      if( stmt->suppl.part.stop_false == 0 ) {
        statement_db_write_tree( stmt->next_false, ofile );
      }
      if( stmt->suppl.part.stop_true == 0 ) {
        statement_db_write_tree( stmt->next_true, ofile );
      }
    }

    /* Output ourselves first */
    statement_db_write( stmt, ofile, TRUE );

  }

  PROFILE_END;

}

/*!
 \param stmt   Pointer to specified statement tree to display
 \param ofile  Pointer to output file to write

 Traverses the specified statement block, writing all expression trees to specified output file.
*/
void statement_db_write_expr_tree(
  statement* stmt,
  FILE*      ofile
) { PROFILE(STATEMENT_DB_WRITE_EXPR_TREE);

  if( stmt != NULL ) {

    /* Output ourselves first */
    expression_db_write_tree( stmt->exp, ofile );

    /* Traverse down the rest of the statement block */
    if( (stmt->next_true == stmt->next_false) && (stmt->suppl.part.stop_true == 0) ) {
      statement_db_write_expr_tree( stmt->next_true, ofile );
    } else {
      if( stmt->suppl.part.stop_false == 0 ) {
        statement_db_write_expr_tree( stmt->next_false, ofile );
      }
      if( stmt->suppl.part.stop_true == 0 ) {
        statement_db_write_expr_tree( stmt->next_true, ofile );
      }
    }

  }

  PROFILE_END;

}

/*!
 \param line        Pointer to current line of file being read.
 \param curr_funit  Pointer to current module.
 \param read_mode   If set to REPORT, adds statement to head of list; otherwise, adds statement to tail.
 
 \throws anonymous Throw Throw

 Reads in the contents of the statement from the specified line, creates
 a statement structure to hold the contents.
*/
void statement_db_read(
  char**     line,
  func_unit* curr_funit,
  int        read_mode
) { PROFILE(STATEMENT_DB_READ);

  int        id;             /* ID of root expression that is associated with this statement */
  int        true_id;        /* ID of root expression that is associated with the next_true statement */
  int        false_id;       /* ID of root expression that is associated with the next_false statement */
  statement* stmt;           /* Pointer to newly created statement */
  exp_link*  expl;           /* Pointer to found expression link */
  stmt_link* stmtl;          /* Pointer to found statement link */
  int        chars_read;     /* Number of characters read from line */
  uint32     suppl;          /* Supplemental field value */

  if( sscanf( *line, "%d %x %d %d%n", &id, &suppl, &true_id, &false_id, &chars_read ) == 4 ) {

    *line = *line + chars_read;

    if( curr_funit == NULL ) {

      print_output( "Internal error:  statement in database written before its functional unit", FATAL, __FILE__, __LINE__ );
      printf( "statement Throw A\n" );
      Throw 0;

    } else {

      /* Find associated root expression */
      expl = exp_link_find( id, curr_funit->exp_head );
      assert( expl != NULL );

      stmt = statement_create( expl->exp, curr_funit );
      stmt->suppl.all = suppl;

      /*
       If this statement is a head statement and the current functional unit is a task, function or named block,
       set the curr_funit->first_stmt pointer to this statement.
      */
      if( (stmt->suppl.part.head == 1) &&
          ((curr_funit->type == FUNIT_TASK)        ||
           (curr_funit->type == FUNIT_ATASK)       ||
           (curr_funit->type == FUNIT_FUNCTION)    ||
           (curr_funit->type == FUNIT_AFUNCTION)   ||
           (curr_funit->type == FUNIT_NAMED_BLOCK) ||
           (curr_funit->type == FUNIT_ANAMED_BLOCK)) ) {
        curr_funit->first_stmt = stmt;
      }

      /* Find and link next_true */
      if( true_id == id ) {
        stmt->next_true = stmt;
      } else if( true_id != 0 ) {
        stmtl = stmt_link_find( true_id, curr_funit->stmt_head );
        if( stmtl == NULL ) {
          /* Add to statement loop queue */
          statement_queue_add( stmt, true_id, TRUE );
        } else {
          stmt->next_true = stmtl->stmt;
        }
        /* Check against statement queue */
        statement_queue_compare( stmt );
      }

      /* Find and link next_false */
      if( false_id == id ) {
        stmt->next_false = stmt;
      } else if( false_id != 0 ) {
        stmtl = stmt_link_find( false_id, curr_funit->stmt_head );
        if( stmtl == NULL ) {
          statement_queue_add( stmt, false_id, FALSE );
        } else {
          stmt->next_false = stmtl->stmt;
        }
        statement_queue_compare( stmt );
      }

      /* Add the statement to the functional unit list */
      if( (read_mode == READ_MODE_NO_MERGE) || (read_mode == READ_MODE_MERGE_NO_MERGE) || (read_mode == READ_MODE_MERGE_INST_MERGE) ) {
        stmt_link_add_tail( stmt, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );
      } else {
        stmt_link_add_head( stmt, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );
      }

      /*
       Possibly add statement to presimulation queue (if the current functional unit is a task
       or function, do not add this to the presimulation queue (this will be added when the expression
       is called.
      */
      if( (read_mode == READ_MODE_NO_MERGE) && (stmt->suppl.part.is_called == 0) ) {
        sim_time tmp_time = {0,0,0,FALSE};
        (void)sim_add_thread( NULL, stmt, curr_funit, &tmp_time );
      }

    }

  } else {

    print_output( "Unable to read statement value", FATAL, __FILE__, __LINE__ );
    printf( "statement Throw B\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param stmt   Pointer to statement block to traverse
 \param funit  Pointer to functional unit containing this statement block

 \throws anonymous statement_assign_expr_ids statement_assign_expr_ids statement_assign_expr_ids expression_assign_expr_ids

 Recursively traverses the entire statement block and assigns unique expression IDs for each
 expression tree that it finds.
*/
void statement_assign_expr_ids(
  statement* stmt,
  func_unit* funit
) { PROFILE(STATEMENT_ASSIGN_EXPR_IDS);

  if( stmt != NULL ) {

    /* Assign unique expression IDs */
    expression_assign_expr_ids( stmt->exp, funit );

    /* Traverse down the rest of the statement block */
    if( (stmt->next_true == stmt->next_false) && (stmt->suppl.part.stop_true == 0) ) {
      statement_assign_expr_ids( stmt->next_true, funit );
    } else {
      if( stmt->suppl.part.stop_false == 0 ) {
        statement_assign_expr_ids( stmt->next_false, funit );
      }
      if( stmt->suppl.part.stop_true == 0 ) {
        statement_assign_expr_ids( stmt->next_true, funit );
      }
    }

  }

  PROFILE_END;

}

#ifdef SKIP
void display( char* id, statement* curr_stmt, statement* next_stmt, int conn_id ) {

  printf( "%s::  curr_stmt id: %d, line: %d, op: %s, conn_id: %d, stop_T: %d, stop_F: %d   next_stmt: %d, line: %d, op: %s   conn_id: %d\n",
          id, curr_stmt->exp->id, curr_stmt->exp->line, expression_string_op( curr_stmt->exp->op ), curr_stmt->conn_id,
          curr_stmt->suppl.part.stop_true, curr_stmt->suppl.part.stop_false, next_stmt->exp->id, next_stmt->exp->line, expression_string_op( next_stmt->exp->op ),
          conn_id );

}
#endif

/*!
 \param curr_stmt  Pointer to statement sequence to traverse.
 \param next_stmt  Pointer to statement to connect ends to.
 \param conn_id    Current connection identifier (used to eliminate infinite looping and connection overwrite)

 \return Returns TRUE if statement was connected to the given statement list; otherwise, returns FALSE.

 Recursively traverses the specified stmt sequence.  When it reaches a statement 
 that has either next_true or next_false set to NULL, sets next_true and/or 
 next_false of that statement to point to the next_stmt statement.
*/
bool statement_connect(
  statement* curr_stmt,
  statement* next_stmt,
  int        conn_id
) { PROFILE(STATEMENT_CONNECT);

  bool retval = FALSE;  /* Return value for this function */

  assert( curr_stmt != NULL );
  assert( next_stmt != NULL );

  /* Specify that this statement has been traversed */
  curr_stmt->conn_id = conn_id;

  //display( "In statement_connect", curr_stmt, next_stmt, conn_id );

  /* If both paths go to the same destination, only parse one path */
  if( curr_stmt->next_true == curr_stmt->next_false ) {
    
    /* If the TRUE path is NULL, connect it to the new statement */
    if( curr_stmt->next_true == NULL ) {
      //display( "Setting next_true to next_stmt", curr_stmt, next_stmt, conn_id );
      curr_stmt->next_true  = next_stmt;
      /* If the current statement is a wait statement, don't connect next_false path */
      if( !EXPR_IS_CONTEXT_SWITCH( curr_stmt->exp ) ) {
        //display( "Setting next_false to next_stmt", curr_stmt, next_stmt, conn_id );
        curr_stmt->next_false = next_stmt;
      }
      if( curr_stmt->next_true->conn_id == conn_id ) {
        //display( "Setting stop_true and stop_false", curr_stmt, next_stmt, conn_id );
        curr_stmt->suppl.part.stop_true  = 1;
        curr_stmt->suppl.part.stop_false = 1;
      } else {
        curr_stmt->next_true->conn_id = conn_id;
      }
      retval = TRUE;
    /* If the TRUE path leads to a loop/merge, set the stop bit and stop traversing */
    } else if( curr_stmt->next_true->conn_id == conn_id ) {
      //display( "Setting stop_true and stop_false", curr_stmt, next_stmt, conn_id );
      curr_stmt->suppl.part.stop_true  = 1;
      curr_stmt->suppl.part.stop_false = 1;
    /* Continue to traverse the TRUE path if the next_stmt does not match this statement */
    } else if( curr_stmt->next_true != next_stmt ) {
      //display( "Traversing next_true path", curr_stmt, next_stmt, conn_id );
      retval |= statement_connect( curr_stmt->next_true, next_stmt, conn_id );
    }

  } else {

    /* Traverse FALSE path */
    if( curr_stmt->next_false == NULL ) {
      if( !EXPR_IS_CONTEXT_SWITCH( curr_stmt->exp ) ) {
        //display( "Setting next_false to next_stmt", curr_stmt, next_stmt, conn_id );
        curr_stmt->next_false = next_stmt;
        if( curr_stmt->next_false->conn_id == conn_id ) {
          //display( "Setting stop_false", curr_stmt, next_stmt, conn_id );
          curr_stmt->suppl.part.stop_false = 1;
        } else {
          curr_stmt->next_false->conn_id = conn_id; 
        }
        retval = TRUE;
      }
    /* If the FALSE path leads to a loop/merge, set the stop bit and stop traversing */
    } else if( curr_stmt->next_false->conn_id == conn_id ) {
      //display( "Setting stop_false", curr_stmt, next_stmt, conn_id );
      curr_stmt->suppl.part.stop_false = 1;
    /* Continue to traverse the FALSE path if the next statement does not match this statement */
    } else if( (curr_stmt->next_false != next_stmt) ) {
      //display( "Traversing next_false path", curr_stmt, next_stmt, conn_id );
      retval |= statement_connect( curr_stmt->next_false, next_stmt, conn_id );
    }

    /* Traverse TRUE path */
    if( curr_stmt->next_true == NULL ) {
      //display( "Setting next_true to next_stmt", curr_stmt, next_stmt, conn_id );
      curr_stmt->next_true = next_stmt;
      if( curr_stmt->next_true->conn_id == conn_id ) {
        //display( "Setting stop_true", curr_stmt, next_stmt, conn_id );
        curr_stmt->suppl.part.stop_true = 1;
      } else {
        curr_stmt->next_true->conn_id = conn_id;
      }
      retval = TRUE;
    /* If the TRUE path leads to a loop/merge, set the stop bit and stop traversing */
    } else if( curr_stmt->next_true->conn_id == conn_id ) {
      //display( "Setting stop_true", curr_stmt, next_stmt, conn_id );
      curr_stmt->suppl.part.stop_true = 1;
    /* Continue to traverse the TRUE path if the next statement does not match this statement */
    } else if( curr_stmt->next_true != next_stmt ) {
      //display( "Traversing next_true path", curr_stmt, next_stmt, conn_id );
      retval |= statement_connect( curr_stmt->next_true, next_stmt, conn_id );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param stmt  Pointer to current statement to look at.
 \param base  Pointer to root statement in statement tree.

 \return Returns the last line number of the specified statement tree.

 Recursively iterates through the specified statement tree searching for the last
 statement in each false/true path (the one whose next pointer points to the head
 statement).  Once it is found, its expression is parsed for its last line and this
 value is returned.  If both the false and tru paths have been parsed, the highest
 numbered line is returned.
*/
static int statement_get_last_line_helper(
  statement* stmt,
  statement* base
) { PROFILE(STATEMENT_GET_LAST_LINE_HELPER);

  expression* last_exp;         /* Pointer to last expression in the statement tree */
  int         last_false = -1;  /* Last false path line number */ 
  int         last_true  = -1;  /* Last true path line number */

  if( stmt != NULL ) {

    /* Check out/traverse false path */
    if( (stmt->next_false == NULL) || (stmt->next_false == base) ) {
      last_exp   = expression_get_last_line_expr( stmt->exp );
      last_false = last_exp->line;
    } else if( stmt->suppl.part.stop_false == 0 ) {
      last_false = statement_get_last_line_helper( stmt->next_false, base );
    }

    /* Check out/traverse true path */
    if( (stmt->next_true == NULL) || (stmt->next_true == base) ) {
      last_exp  = expression_get_last_line_expr( stmt->exp );
      last_true = last_exp->line;
    } else if( stmt->suppl.part.stop_true == 0 ) {
      last_true = statement_get_last_line_helper( stmt->next_true, base );
    }

  }

  PROFILE_END;

  /* Return the greater of the two path last lines */
  return( (last_false > last_true) ? last_false : last_true );

}

/*!
 \return Returns the last line number in the given statement.
*/
int statement_get_last_line(
  statement* stmt  /*!< Pointer to statement to get last line number for */
) { PROFILE(STATEMENT_GET_LAST_LINE);

  int retval = statement_get_last_line_helper( stmt, stmt );

  PROFILE_END;

  return( retval );

}

/*!
 Searches the specified statement block and returns a list of all signals on the right-hand-side
 of expressions.
*/
void statement_find_rhs_sigs(
            statement* stmt,  /*!< Pointer to current statement block to traverse */
  /*@out@*/ str_link** head,  /*!< Pointer to head of signal name list that will contain a list of all RHS signals */
  /*@out@*/ str_link** tail   /*!< Pointer to tail of signal name list that will contain a list of all RHS signals */
) { PROFILE(STATEMENT_FIND_RHS_SIGS);

  if( stmt != NULL ) {

    if( (stmt->exp->op == EXP_OP_NB_CALL) || (stmt->exp->op == EXP_OP_FORK) ) {

      statement_find_rhs_sigs( stmt->exp->elem.funit->first_stmt, head, tail );

    } else {

      /* Find all RHS signals in this statement's expression tree */
      expression_find_rhs_sigs( stmt->exp, head, tail );

    }

    /* If both true and false paths lead to same statement, just traverse the true path */
    if( stmt->next_true == stmt->next_false ) {

      if( stmt->suppl.part.stop_true == 0 ) {
        statement_find_rhs_sigs( stmt->next_true, head, tail );
      }

    /* Otherwise, traverse both true and false paths */
    } else {

      if( stmt->suppl.part.stop_true == 0 ) {
        statement_find_rhs_sigs( stmt->next_true, head, tail );
      }

      if( stmt->suppl.part.stop_false == 0 ) {
        statement_find_rhs_sigs( stmt->next_false, head, tail );
      }

    }

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the head statement of the block that contains stmt.
*/
statement* statement_find_head_statement(
  statement* stmt,  /*!< Pointer to child statement of statement block to find head statement for */
  stmt_link* head   /*!< Pointer to head of statement link list */
) { PROFILE(STATEMENT_FIND_HEAD_STATEMENT);

  stmt_iter  si;     /* Statement iterator used to find head statement */
  statement* fhead;  /* Pointer to found head statement */

  assert( stmt != NULL );

  /* If the specified statement is the head statement, just return it */
  if( stmt->suppl.part.head == 1 ) {

    fhead = stmt;

  } else {

    /* Find statement in statement linked list */
    stmt_iter_reset( &si, head );
    while( (si.curr != NULL) && (si.curr->stmt != stmt) ) {
      stmt_iter_next( &si );
    }

    assert( si.curr != NULL );

    /* Find the head statement using the statement iterator */
    stmt_iter_find_head( &si, FALSE );

    assert( si.curr != NULL );

    fhead = si.curr->stmt;

  }

  PROFILE_END;

  return( fhead );

}

/*!
 \return Returns a pointer to the found statement found within the given statement block.
         If the statement ID could not be found, returns NULL.

 Recursively searches the given statement block for the expression that matches the given
 ID.
*/
statement* statement_find_statement(
  statement* curr,  /*!< Pointer to current statement in statement block being evaluated */
  int        id     /*!< Statement ID to find */
) { PROFILE(STATEMENT_FIND_STATEMENT);

  statement* found = NULL;  /* Pointer to found statement */

  if( curr != NULL ) {

    if( curr->exp->id == id ) {

      found = curr;

    } else {

      /* If both true and false paths lead to same item, just traverse the true path */
      if( curr->next_true == curr->next_false ) {

        if( curr->suppl.part.stop_true == 0 ) {
          found = statement_find_statement( curr->next_true, id );
        }

      /* Otherwise, traverse both true and false paths */
      } else if( (curr->suppl.part.stop_true == 0) &&
                 ((found = statement_find_statement( curr->next_true, id )) == NULL) ) {

        if( curr->suppl.part.stop_false == 0 ) {
          found = statement_find_statement( curr->next_false, id );
        }

      }

    }

  }

  PROFILE_END;

  return( found );

}

/*!
 \return Returns a pointer to the found statement or returns NULL if it was not found.
*/
statement* statement_find_statement_by_position(
  statement*   curr,         /*!< Pointer to current statement in statement block being evaluated */
  unsigned int first_line,   /*!< First line number of statement to find */
  unsigned int first_column  /*!< First column of statement to find */
) { PROFILE(STATEMENT_FIND_STATEMENT_BY_POSITION);

  statement* found = NULL;  /* Pointer to found statement */

  if( curr != NULL ) {

    if( (curr->exp->line == first_line) && (((curr->exp->col >> 16) & 0xffff) == first_column) ) {

      found = curr;

    } else {

      /* If both true and false paths lead to same item, just traverse the true path */
      if( curr->next_true == curr->next_false ) {

        if( curr->suppl.part.stop_true == 0 ) {
          found = statement_find_statement_by_position( curr->next_true, first_line, first_column );
        }

      /* Otherwise, traverse both true and false paths */
      } else if( (curr->suppl.part.stop_true == 0) &&
                 ((found = statement_find_statement_by_position( curr->next_true, first_line, first_column )) == NULL) ) {

        if( curr->suppl.part.stop_false == 0 ) {
          found = statement_find_statement_by_position( curr->next_false, first_line, first_column );
        }

      }

    }

  }

  PROFILE_END;

  return( found );

}

/*!
 \return Returns TRUE if the given statement contains the given expression; otherwise, returns FALSE.
*/
bool statement_contains_expr_calling_stmt(
  statement* curr,  /*!< Pointer to current statement to traverse */
  statement* stmt   /*!< Pointer to statement to find in the associated expression tree */
) { PROFILE(STATEMENT_CONTAINS_EXPR_CALLING_STMT);

  bool contains = (curr != NULL) &&
                  (expression_contains_expr_calling_stmt( curr->exp, stmt ) ||
                  ((curr->suppl.part.stop_true == 0) &&
                    statement_contains_expr_calling_stmt( curr->next_true, stmt )) ||
                  ((curr->next_false != curr->next_false) &&
                   (curr->suppl.part.stop_false == 0) &&
                    statement_contains_expr_calling_stmt( curr->next_false, stmt )));

  PROFILE_END;

  return( contains );

}

/*!
 Recursively deallocates specified statement tree.
*/
void statement_dealloc_recursive(
  statement* stmt,        /*!< Pointer to head of statement tree to deallocate */
  bool       rm_stmt_blk  /*!< If set to TRUE, removes the statement block this statement points to (if any) */
) { PROFILE(STATEMENT_DEALLOC_RECURSIVE);
    
  if( stmt != NULL ) {
  
    assert( stmt->exp != NULL );

    /* If we are a named block or fork call statement, remove that statement block */
    if( (stmt->exp->op == EXP_OP_NB_CALL) || (stmt->exp->op == EXP_OP_FORK) ) {

      if( rm_stmt_blk && (ESUPPL_TYPE( stmt->exp->suppl ) == ETYPE_FUNIT) && (stmt->exp->elem.funit->type != FUNIT_NO_SCORE) ) {
        stmt_blk_add_to_remove_list( stmt->exp->elem.funit->first_stmt );
      }

    }

    /* Remove TRUE path */
    if( stmt->next_true == stmt->next_false ) {

      if( stmt->suppl.part.stop_true == 0 ) {
        statement_dealloc_recursive( stmt->next_true, rm_stmt_blk );
      }

    } else {

      if( stmt->suppl.part.stop_true == 0 ) {
        statement_dealloc_recursive( stmt->next_true, rm_stmt_blk );
      }
  
      /* Remove FALSE path */
      if( stmt->suppl.part.stop_false == 0 ) {
        statement_dealloc_recursive( stmt->next_false, rm_stmt_blk );
      }

    }

    /* Disconnect statement from current functional unit */
    db_remove_statement_from_current_funit( stmt );

    free_safe( stmt, sizeof( statement ) );
    
  }

  PROFILE_END;
  
}

/*!
 Deallocates specified statement from heap memory.  Does not
 remove attached expression (this is assumed to be cleaned up by the
 expression list removal function).
*/
void statement_dealloc(
  statement* stmt  /*!< Pointer to statement to deallocate */
) { PROFILE(STATEMENT_DEALLOC);

  if( stmt != NULL ) {
 
    /* Finally, deallocate this statement */
    free_safe( stmt, sizeof( statement ) );

  }

  PROFILE_END;

}


/*
 $Log$
 Revision 1.139  2008/10/31 22:01:34  phase1geo
 Initial code changes to support merging two non-overlapping CDD files into
 one.  This functionality seems to be working but needs regression testing to
 verify that nothing is broken as a result.

 Revision 1.138  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.134.2.1  2008/07/10 22:43:54  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.136  2008/06/27 14:02:04  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.135  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.134  2008/05/30 23:00:48  phase1geo
 Fixing Doxygen comments to eliminate Doxygen warning messages.

 Revision 1.133  2008/04/06 05:24:17  phase1geo
 Fixing another regression memory problem.  Updated regression files
 accordingly.  Checkpointing.

 Revision 1.132  2008/03/31 21:40:24  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.131  2008/03/17 05:26:17  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.130  2008/03/14 22:00:20  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.129  2008/03/11 22:06:49  phase1geo
 Finishing first round of exception handling code.

 Revision 1.128  2008/03/04 00:09:20  phase1geo
 More exception handling.  Checkpointing.

 Revision 1.127  2008/02/29 00:08:31  phase1geo
 Completed optimization code in simulator.  Still need to verify that code
 changes enhanced performances as desired.  Checkpointing.

 Revision 1.126  2008/02/28 07:54:09  phase1geo
 Starting to add functionality for simulation optimization in the sim_expr_changed
 function (feature request 1897410).

 Revision 1.125  2008/02/25 20:43:49  phase1geo
 Checking in code to allow the use of racecheck pragmas.  Added new tests to
 regression suite to verify this functionality.  Still need to document in
 User's Guide and manpage.

 Revision 1.124  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.123  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.122  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.121  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.120  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.119  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.118  2007/12/19 22:54:35  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.117  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.116  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.115  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.114  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.113  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.112  2007/07/26 17:05:15  phase1geo
 Fixing problem with static functions (vector data associated with expressions
 were not being allocated).  Regressions have been run.  Only two failures
 in total still to be fixed.

 Revision 1.111  2007/07/18 02:15:04  phase1geo
 Attempts to fix a problem with generating instances with hierarchy.  Also fixing
 an issue with named blocks in generate statements.  Still some work to go before
 regressions are passing again, however.

 Revision 1.110  2007/04/18 22:35:02  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.109  2007/04/12 20:54:55  phase1geo
 Adding cli > output when replaying and adding back all of the functions (since
 the cli > prompt helps give it context.  Fixing bugs in simulation core.
 Checkpointing.

 Revision 1.108  2007/04/11 22:29:49  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.107  2007/04/10 03:56:18  phase1geo
 Completing majority of code to support new simulation core.  Starting to debug
 this though we still have quite a ways to go here.  Checkpointing.

 Revision 1.106  2007/04/09 22:47:53  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.105  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.104  2007/03/16 22:33:46  phase1geo
 One more fix that helps diagnostics like always1 and still fixes exclude3.
 Regressions are still not working correctly yet, though.

 Revision 1.103  2007/03/16 22:28:14  phase1geo
 Checkpointing again.  Still having quite a few issues with getting good coverage
 reports.  Fixing a few more problems that the exclude3 diagnostic complained
 about.

 Revision 1.102  2007/03/16 21:41:10  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.101  2007/03/08 05:17:30  phase1geo
 Various code fixes.  Full regression does not yet pass.

 Revision 1.100  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.99  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.98  2006/12/14 04:19:35  phase1geo
 More updates to parser and associated code to handle unnamed scopes and
 fixing more code to use functional unit pointers in expressions instead of
 statement pointers.  Still not fully compiling at this point.  Checkpointing.

 Revision 1.97  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.96  2006/10/04 22:04:16  phase1geo
 Updating rest of regressions.  Modified the way we are setting the memory rd
 vector data bit (should optimize the score command just a bit).  Also updated
 quite a bit of memory coverage documentation though I still need to finish
 documenting how to understand the report file for this metric.  Cleaning up
 other things and fixing a few more software bugs from regressions.  Added
 marray2* diagnostics to verify endianness in the unpacked dimension list.

 Revision 1.95  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.94  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.93  2006/09/08 14:56:05  phase1geo
 Somehow a return from the statement_find_statement function was missing that
 caused problems with removing statement blocks.

 Revision 1.92  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.91  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.90  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.89  2006/08/16 15:32:15  phase1geo
 Fixing issues with do..while loop handling.  Full regression now passes.

 Revision 1.88  2006/08/11 21:27:10  phase1geo
 Adding support for unique, priority and do..while SystemVerilog constructs.
 do_while2 diagnostic is currently failing with an issue regarding connecting its
 false path to the top of its always block.  Otherwise, full regression should
 be passing (with the exception of the problem with the assigned bit due to changes
 for generate11).

 Revision 1.87  2006/08/10 22:35:14  phase1geo
 Updating with fixes for upcoming 0.4.7 stable release.  Updated regressions
 for this change.  Full regression still fails due to an unrelated issue.

 Revision 1.86  2006/07/28 22:42:51  phase1geo
 Updates to support expression/signal binding for expressions within a generate
 block statement block.

 Revision 1.85  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.84  2006/07/22 01:17:22  phase1geo
 Fixing generate statement output function to output statements in the correct order.
 Diagnostic generate5.v should now work correctly with VCS.

 Revision 1.83  2006/07/21 22:39:01  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.82  2006/07/18 21:52:49  phase1geo
 More work on generate blocks.  Currently working on assembling generate item
 statements in the parser.  Still a lot of work to go here.

 Revision 1.81  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.80  2006/06/26 22:49:00  phase1geo
 More updates for exclusion of combinational logic.  Also updates to properly
 support CDD saving; however, this change causes regression errors, currently.

 Revision 1.79  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.78  2006/05/25 12:11:01  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.77  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.76.4.1  2006/04/20 18:48:56  phase1geo
 Fixing bug in statement_connect function to force the last statement of a true
 path to get the stop_true and stop_false bits get set.

 Revision 1.76  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.75  2006/03/15 22:48:29  phase1geo
 Updating run program.  Fixing bugs in statement_connect algorithm.  Updating
 regression files.

 Revision 1.74  2006/03/13 21:10:06  phase1geo
 Fixing statement connection issue when reading in a CDD file.  There were cases
 that would cause connection lossage due to the statement reconnection algorithm.

 Revision 1.73  2006/02/10 16:44:29  phase1geo
 Adding support for register assignment.  Added diagnostic to regression suite
 to verify its implementation.  Updated TODO.  Full regression passes at this
 point.

 Revision 1.72  2006/01/24 23:24:38  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.71  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.70  2006/01/10 23:13:51  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.69  2006/01/06 23:39:10  phase1geo
 Started working on removing the need to simulate more than is necessary.  Things
 are pretty broken at this point, but all of the code should be in -- debugging.

 Revision 1.68  2006/01/06 18:54:03  phase1geo
 Breaking up expression_operate function into individual functions for each
 expression operation.  Also storing additional information in a globally accessible,
 constant structure array to increase performance.  Updating full regression for these
 changes.  Full regression passes.

 Revision 1.67  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.66  2005/12/23 20:59:34  phase1geo
 Fixing assertion error in race condition checker.  Full regression runs cleanly.

 Revision 1.65  2005/12/10 06:41:18  phase1geo
 Added support for FOR loops and added diagnostics to regression suite to verify
 functionality.  Fixed statement deallocation function (removed a bunch of code
 there now that statement stopping is working as intended).  Full regression passes.

 Revision 1.64  2005/12/08 19:47:00  phase1geo
 Fixed repeat2 simulation issues.  Fixed statement_connect algorithm, removed the
 need for a separate set_stop function and reshuffled the positions of esuppl bits.
 Full regression passes.

 Revision 1.63  2005/12/07 20:23:38  phase1geo
 Fixing case where statement is unconnectable.  Full regression now passes.

 Revision 1.62  2005/12/05 20:26:55  phase1geo
 Fixing bugs in code to remove statement blocks that are pointed to by expressions
 in NB_CALL and FORK cases.  Fixed bugs in fork code -- this is now working at the
 moment.  Updated regressions which now fully pass.

 Revision 1.61  2005/11/30 18:25:56  phase1geo
 Fixing named block code.  Full regression now passes.  Still more work to do on
 named blocks, however.

 Revision 1.60  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.59  2005/11/29 19:04:48  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.58  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.57  2005/11/25 22:03:20  phase1geo
 Fixing bugs in race condition checker when racing statement blocks are in
 different functional units.  Still some work to do here with what to do when
 conflicting statement block is in a task/function (I suppose we need to remove
 the calling statement block as well?)

 Revision 1.56  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.55  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.54  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.53  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.52  2005/02/07 05:10:15  phase1geo
 Fixing bug in statement_get_last_line calculator.  Updated regression for this
 fix.

 Revision 1.51  2005/02/04 23:55:54  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.50  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.49  2004/07/22 04:43:06  phase1geo
 Finishing code to calculate start and end columns of expressions.  Regression
 has been updated for these changes.  Other various minor changes as well.

 Revision 1.48  2004/03/16 14:01:47  phase1geo
 Cleaning up verbose output.

 Revision 1.47  2004/03/16 13:56:05  phase1geo
 Fixing bug with statement removal to make it less prone to segmentation faulting.

 Revision 1.46  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.45  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.44  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.43  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.42  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.41  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.40  2003/02/08 21:54:07  phase1geo
 Fixing memory problems with db_remove_statement function.  Updating comments
 in statement.c to explain some of the changes necessary to properly remove
 a statement tree.

 Revision 1.39  2002/12/13 16:49:56  phase1geo
 Fixing infinite loop bug with statement set_stop function.  Removing
 hierarchical references from scoring (same problem as defparam statement).
 Fixing problem with checked in version of param.c and fixing error output
 in bind() function to be more meaningful to user.

 Revision 1.38  2002/12/07 17:46:53  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.37  2002/12/05 20:43:01  phase1geo
 Fixing bug in statement_set_stop function that caused infinite looping to occur.
 Added diagnostic to verify fix.

 Revision 1.36  2002/12/03 06:01:18  phase1geo
 Fixing bug where delay statement is the last statement in a statement list.
 Adding diagnostics to verify this functionality.

 Revision 1.35  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.34  2002/10/31 23:14:26  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.33  2002/10/30 06:07:11  phase1geo
 First attempt to handle expression trees/statement trees that contain
 unsupported code.  These are removed completely and not evaluated (because
 we can't guarantee that our results will match the simulator).  Added real1.1.v
 diagnostic that verifies one case of this scenario.  Full regression passes.

 Revision 1.32  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.31  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.30  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.29  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.27  2002/07/12 04:53:29  phase1geo
 Removing counter code that was used for debugging infinite loops in code
 previously.

 Revision 1.26  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.25  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.24  2002/07/04 23:10:12  phase1geo
 Added proper support for case, casex, and casez statements in score command.
 Report command still incorrect for these statement types.

 Revision 1.23  2002/07/03 21:30:53  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.22  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.21  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.20  2002/06/30 22:23:20  phase1geo
 Working on fixing looping in parser.  Statement connector needs to be revamped.

 Revision 1.18  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.17  2002/06/28 00:40:37  phase1geo
 Cleaning up extraneous output from debugging.

 Revision 1.16  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.15  2002/06/27 12:36:47  phase1geo
 Fixing bugs with scoring.  I think I got it this time.

 Revision 1.14  2002/06/26 22:09:17  phase1geo
 Removing unecessary output and updating regression Makefile.

 Revision 1.13  2002/06/26 03:45:48  phase1geo
 Fixing more bugs in simulator and report functions.  About to add support
 for delay statements.

 Revision 1.12  2002/06/25 21:46:10  phase1geo
 Fixes to simulator and reporting.  Still some bugs here.

 Revision 1.11  2002/06/25 12:48:38  phase1geo
 Fixing case where statement's true and false paths point to itself when
 reading in CDD.

 Revision 1.10  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.9  2002/06/25 02:02:04  phase1geo
 Fixing bugs with writing/reading statements and with parsing design with
 statements.  We now get to the scoring section.  Some problems here at
 the moment with the simulator.

 Revision 1.7  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.6  2002/06/23 21:18:22  phase1geo
 Added appropriate statement support in parser.  All parts should be in place
 and ready to start testing.

 Revision 1.5  2002/06/22 05:27:30  phase1geo
 Additional supporting code for simulation engine and statement support in
 parser.

 Revision 1.4  2002/06/21 05:55:05  phase1geo
 Getting some codes ready for writing simulation engine.  We should be set
 now.

 Revision 1.3  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.2  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.

 Revision 1.1  2002/05/02 03:27:42  phase1geo
 Initial creation of statement structure and manipulation files.  Internals are
 still in a chaotic state.
*/

