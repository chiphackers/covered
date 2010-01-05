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
#include "stmt_blk.h"


extern char     user_msg[USER_MSG_LENGTH];
extern exp_info exp_op_info[EXP_OP_NUM];
extern isuppl   info_suppl;

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
  expression*  exp,   /*!< Pointer to root expression of expression tree for this statement */
  func_unit*   funit  /*!< Pointer to functional unit that this statement exists in */
) { PROFILE(STATEMENT_CREATE);

  statement* stmt;  /* Pointer to newly created statement */

  stmt                    = (statement*)malloc_safe( sizeof( statement ) );
  stmt->exp               = exp;
  stmt->exp->parent->stmt = stmt;
  stmt->next_true         = NULL;
  stmt->next_false        = NULL;
  stmt->head              = NULL;
  stmt->conn_id           = 0;
  stmt->suppl.all         = 0;
  stmt->funit             = funit;

  PROFILE_END;

  return( stmt );

}

#ifndef RUNLIB
/*!
 Displays the current contents of the statement loop list for debug purposes only.
*/
void statement_queue_display() {

  stmt_loop_link* sll;  /* Pointer to current statement loop link */

  printf( "Statement loop list:\n" );

  sll = stmt_loop_head;
  while( sll != NULL ) {
    printf( "  id: %d, type: %d, stmt: %s  ", sll->id, sll->type, expression_string( sll->stmt->exp ) );
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
#endif /* RUNLIB */

/*!
 Creates a new statement loop link for the specified parameters and adds this
 element to the top of the statement loop queue.
*/
static void statement_queue_add(
  statement* stmt,  /*!< Pointer of statement waiting to be linked */
  int        id,    /*!< ID of statement to be read out later */
  int        type   /*!< Specifies the type of statement that we are storing */
) { PROFILE(STATEMENT_QUEUE_ADD);

  stmt_loop_link* sll;  /* Pointer to newly created statement loop link */

  /* Create statement loop link element */
  sll = (stmt_loop_link*)malloc_safe( sizeof( stmt_loop_link ) );

  /* Populate statement loop link with specified parameters */
  sll->stmt = stmt;
  sll->id   = id;
  sll->type = type;
  sll->next = NULL;

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
 Compares the specified statement against the top of the statement loop queue.  If
 an ID in the queue matches this statement's ID, the element is removed and the
 next_true and next_false pointers of the stored statement are pointed
 to the specified statement.  The next head is also compared against this statement
 and the process is repeated until a match is not found.
*/
static void statement_queue_compare(
  statement* stmt  /*!< Pointer to statement being read out of the CDD */
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
      if( (sll->stmt->next_true == NULL) && (sll->type == 0) ) {
        sll->stmt->next_true = stmt;
      }
      if( (sll->stmt->next_false == NULL) && (sll->type == 1) ) {
        sll->stmt->next_false = stmt;
      }
      if( (sll->stmt->head == NULL) && (sll->type == 2) ) {
        sll->stmt->head = stmt;
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

#ifndef RUNLIB
/*!
 \throws anonymous expression_resize statement_size_elements statement_size_elements statement_size_elements

 Recursively sizes all elements for the given statement block.
*/
void statement_size_elements(
  statement* stmt,  /*!< Pointer to statement block to size elements for */
  func_unit* funit  /*!< Pointer to functional unit containing this statement block */
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
#endif /* RUNLIB */
    
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
  fprintf( ofile, "%d %d %x %d %d %d",
    DB_TYPE_STATEMENT,
    expression_get_id( stmt->exp, ids_issued ),
    (stmt->suppl.all & 0xff),
    ((stmt->next_true   == NULL) ? 0 : expression_get_id( stmt->next_true->exp, ids_issued )),
    ((stmt->next_false  == NULL) ? 0 : expression_get_id( stmt->next_false->exp, ids_issued )),
    ((stmt->head        == NULL) ? 0 : expression_get_id( stmt->head->exp, ids_issued ))
  );

  fprintf( ofile, "\n" );

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Traverses specified statement tree, outputting all statements within that tree.
*/
void statement_db_write_tree(
  statement* stmt,  /*!< Pointer to root of statement tree to output */
  FILE*      ofile  /*!< Pointer to output file to write statements to */
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
 Traverses the specified statement block, writing all expression trees to specified output file.
*/
void statement_db_write_expr_tree(
  statement* stmt,  /*!< Pointer to specified statement tree to display */
  FILE*      ofile  /*!< Pointer to output file to write */
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
#endif /* RUNLIB */

/*!
 \throws anonymous Throw Throw

 Reads in the contents of the statement from the specified line, creates
 a statement structure to hold the contents.
*/
void statement_db_read(
  char**     line,        /*!< Pointer to current line of file being read */
  func_unit* curr_funit,  /*!< Pointer to current module */
  int        read_mode    /*!< If set to REPORT, adds statement to head of list; otherwise, adds statement to tail */
) { PROFILE(STATEMENT_DB_READ);

  int        id;          /* ID of root expression that is associated with this statement */
  int        true_id;     /* ID of root expression that is associated with the next_true statement */
  int        false_id;    /* ID of root expression that is associated with the next_false statement */
  int        head_id;
  statement* stmt;        /* Pointer to newly created statement */
  stmt_link* stmtl;       /* Pointer to found statement link */
  int        chars_read;  /* Number of characters read from line */
  uint32     suppl;       /* Supplemental field value */

  if( sscanf( *line, "%d %x %d %d %d%n", &id, &suppl, &true_id, &false_id, &head_id, &chars_read ) == 5 ) {

    *line = *line + chars_read;

    if( curr_funit == NULL ) {

      print_output( "Internal error:  statement in database written before its functional unit", FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      /* Find associated root expression */
      expression* exp = exp_link_find( id, curr_funit->exps, curr_funit->exp_size );
      assert( exp != NULL );

      stmt = statement_create( exp, curr_funit );
      stmt->suppl.all = suppl;

      /*
       If this statement is a head statement and the current functional unit is a task, function or named block,
       set the curr_funit->first_stmt pointer to this statement.
      */
      if( (stmt->suppl.part.head == 1) &&
          ((curr_funit->suppl.part.type == FUNIT_TASK)        ||
           (curr_funit->suppl.part.type == FUNIT_ATASK)       ||
           (curr_funit->suppl.part.type == FUNIT_FUNCTION)    ||
           (curr_funit->suppl.part.type == FUNIT_AFUNCTION)   ||
           (curr_funit->suppl.part.type == FUNIT_NAMED_BLOCK) ||
           (curr_funit->suppl.part.type == FUNIT_ANAMED_BLOCK)) ) {
        curr_funit->first_stmt = stmt;
      }

      /* Find and link next_true */
      if( true_id == id ) {
        stmt->next_true = stmt;
      } else if( true_id != 0 ) {
        stmtl = stmt_link_find( true_id, curr_funit->stmt_head );
        if( stmtl == NULL ) {
          /* Add to statement loop queue */
          statement_queue_add( stmt, true_id, 0 );
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
          statement_queue_add( stmt, false_id, 1 );
        } else {
          stmt->next_false = stmtl->stmt;
        }
        statement_queue_compare( stmt );
      }

      /* Find and link head */
      if( head_id == id ) {
        stmt->head = stmt;
      } else if( head_id != 0 ) {
        stmtl = stmt_link_find( head_id, curr_funit->stmt_head );
        if( stmtl == NULL ) {
          statement_queue_add( stmt, head_id, 2 );
        } else {
          stmt->head = stmtl->stmt;
        }
        statement_queue_compare( stmt );
      }

      /* Add the statement to the functional unit list */
      stmt_link_add( stmt, TRUE, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );

#ifndef RUNLIB
      /*
       Possibly add statement to presimulation queue (if the current functional unit is a task
       or function, do not add this to the presimulation queue (this will be added when the expression
       is called.
      */
      if( (read_mode == READ_MODE_NO_MERGE) && (stmt->suppl.part.is_called == 0) && (info_suppl.part.inlined == 0) ) {
        sim_time tmp_time = {0,0,0,FALSE};
        (void)sim_add_thread( NULL, stmt, curr_funit, &tmp_time );
      }
#endif /* RUNLIB */

    }

  } else {

    print_output( "Unable to read statement value", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 \throws anonymous statement_assign_expr_ids statement_assign_expr_ids statement_assign_expr_ids expression_assign_expr_ids

 Recursively traverses the entire statement block and assigns unique expression IDs for each
 expression tree that it finds.
*/
void statement_assign_expr_ids(
  statement* stmt,  /*!< Pointer to statement block to traverse */
  func_unit* funit  /*!< Pointer to functional unit containing this statement block */
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
 \return Returns TRUE if statement was connected to the given statement list; otherwise, returns FALSE.

 Recursively traverses the specified stmt sequence.  When it reaches a statement 
 that has either next_true or next_false set to NULL, sets next_true and/or 
 next_false of that statement to point to the next_stmt statement.
*/
bool statement_connect(
  statement* curr_stmt,  /*!< Pointer to statement sequence to traverse */
  statement* next_stmt,  /*!< Pointer to statement to connect ends to */
  int        conn_id     /*!< Current connection identifier (used to eliminate infinite looping and connection overwrite) */
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
 \return Returns the last line number of the specified statement tree.

 Recursively iterates through the specified statement tree searching for the last
 statement in each false/true path (the one whose next pointer points to the head
 statement).  Once it is found, its expression is parsed for its last line and this
 value is returned.  If both the false and tru paths have been parsed, the highest
 numbered line is returned.
*/
static int statement_get_last_line_helper(
  statement* stmt,  /*!< Pointer to current statement to look at */
  statement* base   /*!< Pointer to root statement in statement tree */
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

    if( (curr->exp->line == first_line) && (curr->exp->col.part.first == first_column) ) {

      found = curr;

    } else if( (curr->exp->op == EXP_OP_NB_CALL) || (curr->exp->op == EXP_OP_FORK) ) {

      found = statement_find_statement_by_position( curr->exp->elem.funit->first_stmt, first_line, first_column );

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
 Recursively traverses the entire statement block and adds the given statements to the specified statement list.
*/
void statement_add_to_stmt_link(
  statement*  stmt,  /*!< Pointer to statement block to traverse */
  stmt_link** head,  /*!< Pointer to head of stmt_link list */
  stmt_link** tail   /*!< Pointer to tail of stmt_link list */
) { PROFILE(STATEMENT_ADD_TO_STMT_LINK);

  if( stmt != NULL ) {

    /* Add the current statement to the stmt_link list */
    stmt_link_add( stmt, FALSE, head, tail );

    /* Traverse down the rest of the statement block */
    if( (stmt->next_true == stmt->next_false) && (stmt->suppl.part.stop_true == 0) ) {
      statement_add_to_stmt_link( stmt->next_true, head, tail );
    } else {
      if( stmt->suppl.part.stop_false == 0 ) {
        statement_add_to_stmt_link( stmt->next_false, head, tail );
      }
      if( stmt->suppl.part.stop_true == 0 ) {
        statement_add_to_stmt_link( stmt->next_true, head, tail );
      }
    }

  }

  PROFILE_END;

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

      if( rm_stmt_blk && (ESUPPL_TYPE( stmt->exp->suppl ) == ETYPE_FUNIT) && (stmt->exp->elem.funit->suppl.part.type != FUNIT_NO_SCORE) ) {
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
#endif /* RUNLIB */

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

