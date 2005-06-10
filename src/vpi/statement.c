/*!
 \file     statement.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     5/1/2002

 \par
 Statements are used to indicate flow of execution for a given always or initial
 block.  Each statement is assigned to exactly one expression tree and it
 contains a pointer to the next statement if its expression tree evaluates to
 TRUE (non-zero value) or FALSE (zero value).  To minimize memory use, a statement 
 uses some of the unused bits in its root expression supplemental field instead of
 having its own supplemental field.  There are two bits in the expression
 supplemental value that are only used by statements:  #ESUPPL_LSB_STMT_HEAD and
 #ESUPPL_LSB_STMT_STOP.

 \par
 The STMT_HEAD bit indicates that this statement should be loaded into the
 pre-simulation statement queue if any of its expressions change value in the
 current timestep.  To begin with, the first statement in the always/initial block
 has this bit set, all other statements have this bit cleared.

 \par
 The STMT_STOP bit is used for CDD output.  If a statement has this bit set, it will
 not traverse its next_true or next_false paths when outputting.  This is necessary
 when statement paths merge back to the same path.  If this bit was not used, the
 statements output after the merge would be output twice.  Consider the following
 Verilog code snippet:

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
 To eliminate this redundancy, the STMT_STOP bit is set on statement 4.  The last
 statement of a TRUE path before merging always gets its STMT_STOP bit set.  The
 FALSE path should never get the STMT_STOP bit set.  Additionally, once a statement
 gets its STMT_STOP bit set by the parser, this value must be maintained (never
 cleared).

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
 will receive the STMT_HEAD bit and the current statement tree is finished for this
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


extern char user_msg[USER_MSG_LENGTH];

/*!
 Pointer to statement loop stack structure.  See description of \ref stmt_loop_link
 for more information on this stack structure usage.
*/
stmt_loop_link* stmt_loop_stack = NULL;


/*!
 \param exp   Pointer to root expression of expression tree for this statement.
  
 \return Returns pointer to the newly created statement.

 Creates a new statement structure from heap memory and initializes it with the
 specified parameter information.
*/
statement* statement_create( expression* exp ) {

  statement* stmt;  /* Pointer to newly created statement */

  stmt                       = (statement*)malloc_safe( sizeof( statement ), __FILE__, __LINE__ );
  stmt->exp                  = exp;
  stmt->exp->parent->stmt    = stmt;
  stmt->exp->suppl.part.root = 1;
  stmt->wait_sig_head        = NULL;
  stmt->wait_sig_tail        = NULL;
  stmt->next_true            = NULL;
  stmt->next_false           = NULL;

  expression_get_wait_sig_list( exp, &(stmt->wait_sig_head), &(stmt->wait_sig_tail) );

  return( stmt );

}

/*!
 \param stmt  Pointer of statement waiting to be linked.
 \param id    ID of statement to be read out later.

 Creates a new statement loop link for the specified parameters and adds this
 element to the top of the statement loop stack.
*/
void statement_stack_push( statement* stmt, int id ) {

  stmt_loop_link* sll;     /* Pointer to newly created statement loop link */

  /* Create statement loop link element */
  sll = (stmt_loop_link*)malloc_safe( sizeof( stmt_loop_link ), __FILE__, __LINE__ );

  /* Populate statement loop link with specified parameters */
  sll->stmt = stmt;
  sll->id   = id;
  sll->next = NULL;

  /* Add to top of statement loop stack */
  if( stmt_loop_stack == NULL ) {
    stmt_loop_stack = sll;
  } else {
    sll->next       = stmt_loop_stack;
    stmt_loop_stack = sll;
  }

}

/*!
 \param stmt  Pointer to statement being read out of the CDD.
 
 Compares the specified statement against the top of the statement loop stack.  If
 the ID at the top of the stack matches this statement's ID, the top of the stack is
 popped and the next_true and next_false pointers of the stored statement are pointed
 to the specified statement.  The next head is also compared against this statement
 and the process is repeated until a match is not found.  Once an ID at the top of the
 stack does not match, no further action is taken.
*/
void statement_stack_compare( statement* stmt ) {

  stmt_loop_link* sll;    /* Pointer to top of stack */

  while( (stmt_loop_stack != NULL) && (stmt->exp->id == stmt_loop_stack->id) ) {

    /* Perform the link */
    if( stmt_loop_stack->stmt->next_true == NULL ) {
      stmt_loop_stack->stmt->next_true  = stmt;
    }
    if( stmt_loop_stack->stmt->next_false == NULL ) {
      stmt_loop_stack->stmt->next_false = stmt;
    }

    /* Pop the top off of the stack */
    sll             = stmt_loop_stack;
    stmt_loop_stack = stmt_loop_stack->next;

    /* Deallocate the memory for the link */
    free_safe( sll );

  }

}
    
/*!
 \param stmt   Pointer to statement to write out value.
 \param ofile  Pointer to output file to write statement line to.

 Recursively writes the contents of the specified statement tree (and its
 associated expression trees to the specified output stream.
*/
void statement_db_write( statement* stmt, FILE* ofile ) {

  assert( stmt != NULL );

#ifdef EFFICIENCY_CODE
  /* Write succeeding statements first */
  if( ESUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0 ) {

    statement_db_write( stmt->next_true, ofile );
    statement_db_write( stmt->next_false, ofile );

  }

  assert( stmt->exp != NULL );

  /* Write out expression tree second */
  expression_db_write( stmt->exp, ofile );
#endif

  /* Write out contents of this statement last */
  fprintf( ofile, "%d %d",
    DB_TYPE_STATEMENT,
    stmt->exp->id
  );

  if( stmt->next_true == NULL ) {
    fprintf( ofile, " 0" );
  } else {
    fprintf( ofile, " %d", stmt->next_true->exp->id );
  }

  if( stmt->next_false == NULL ) {
    fprintf( ofile, " 0" );
  } else {
    fprintf( ofile, " %d", stmt->next_false->exp->id );
  }

  fprintf( ofile, "\n" );

}

/*!
 \param line      Pointer to current line of file being read.
 \param curr_mod  Pointer to current module.
 \param read_mode  If set to REPORT, adds statement to head of list; otherwise, adds statement to tail.
 
 \return Returns TRUE if the line is read without error; otherwise, returns FALSE.

 Reads in the contents of the statement from the specified line, creates
 a statement structure to hold the contents.
*/
bool statement_db_read( char** line, module* curr_mod, int read_mode ) {

  bool       retval = TRUE;  /* Return value of this function                                          */
  int        id;             /* ID of root expression that is associated with this statement           */
  int        true_id;        /* ID of root expression that is associated with the next_true statement  */
  int        false_id;       /* ID of root expression that is associated with the next_false statement */
  expression tmpexp;         /* Temporary expression used for expression search                        */
  statement* stmt;           /* Pointer to newly created statement                                     */
  exp_link*  expl;           /* Pointer to found expression link                                       */
  stmt_link* stmtl;          /* Pointer to found statement link                                        */
  int        chars_read;     /* Number of characters read from line                                    */

  if( sscanf( *line, "%d %d %d%n", &id, &true_id, &false_id, &chars_read ) == 3 ) {

    *line = *line + chars_read;

    if( curr_mod == NULL ) {

      print_output( "Internal error:  statement in database written before its module", FATAL, __FILE__, __LINE__ );
      retval = FALSE;

    } else {

      /* Find associated root expression */
      tmpexp.id = id;
      expl = exp_link_find( &tmpexp, curr_mod->exp_head );

      stmt = statement_create( expl->exp );

      /* Find and link next_true */
      if( true_id == id ) {
        stmt->next_true = stmt;
      } else if( true_id != 0 ) {
        stmtl = stmt_link_find( true_id, curr_mod->stmt_head );
        if( stmtl == NULL ) {
          /* Add to statement loop stack */
          statement_stack_push( stmt, true_id );
        } else {
          /* Check against statement stack */
          statement_stack_compare( stmt );
          stmt->next_true = stmtl->stmt;
        }
      }

      /* Find and link next_false */
      if( false_id == id ) {
        stmt->next_false = stmt;
      } else if( false_id != 0 ) {
        stmtl = stmt_link_find( false_id, curr_mod->stmt_head );
        if( stmtl == NULL ) {
          statement_stack_push( stmt, false_id );
        } else {
          statement_stack_compare( stmt );
          stmt->next_false = stmtl->stmt;
        }
      }

      /* Add statement to module statement list */
      if( (read_mode == READ_MODE_MERGE_NO_MERGE) || (read_mode == READ_MODE_MERGE_INST_MERGE) ) {
        stmt_link_add_tail( stmt, &(curr_mod->stmt_head), &(curr_mod->stmt_tail) );
      } else {
        stmt_link_add_head( stmt, &(curr_mod->stmt_head), &(curr_mod->stmt_tail) );
      }

      /* Possibly add statement to presimulation queue */
      sim_add_stmt_to_queue( stmt );

    }

  } else {

    print_output( "Unable to read statement value", FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param curr_stmt  Pointer to statement sequence to traverse.
 \param next_stmt  Pointer to statement to connect ends to.

 Recursively traverses the specified stmt sequence.  When it reaches a statement 
 that has either next_true or next_false set to NULL, sets next_true and/or 
 next_false of that statement to point to the next_stmt statement.
*/
void statement_connect( statement* curr_stmt, statement* next_stmt ) {

  assert( curr_stmt != NULL );
  assert( next_stmt != NULL );
    
  /* If both paths go to the same destination, only parse one path */
  if( (curr_stmt->next_true == curr_stmt->next_false) || 
      (curr_stmt->exp->suppl.part.stmt_connected == 1) ) {

    if( curr_stmt->next_true == NULL ) {
      curr_stmt->next_true  = next_stmt;
      /* If the current statement is a wait statement, don't connect next_false path */
      if( (curr_stmt->exp->op != EXP_OP_DELAY) &&
          (curr_stmt->exp->op != EXP_OP_NEDGE) &&
          (curr_stmt->exp->op != EXP_OP_PEDGE) &&
          (curr_stmt->exp->op != EXP_OP_AEDGE) &&
          (curr_stmt->exp->op != EXP_OP_EOR) ) {
        curr_stmt->next_false = next_stmt;
      }
    } else if( curr_stmt->next_true != next_stmt ) {
      statement_connect( curr_stmt->next_true, next_stmt );
    }

  } else {

    /* Traverse TRUE path */
    if( curr_stmt->next_true == NULL ) {
      curr_stmt->next_true = next_stmt;
    } else if( curr_stmt->next_true != next_stmt ) {
      statement_connect( curr_stmt->next_true, next_stmt );
    }

    /* Traverse FALSE path */
    if( curr_stmt->next_false == NULL ) {
      if( (curr_stmt->exp->op != EXP_OP_DELAY) &&
          (curr_stmt->exp->op != EXP_OP_NEDGE) &&
          (curr_stmt->exp->op != EXP_OP_PEDGE) &&
          (curr_stmt->exp->op != EXP_OP_AEDGE) &&
          (curr_stmt->exp->op != EXP_OP_EOR) ) {
        curr_stmt->next_false = next_stmt;
      }
    } else if( curr_stmt->next_false != next_stmt ) {
      statement_connect( curr_stmt->next_false, next_stmt );
    }

  }

  if( (curr_stmt->next_true != NULL) && (curr_stmt->next_false != NULL) ) {
    curr_stmt->exp->suppl.part.stmt_connected = 1;
  }

}

/*!
 \param stmt       Pointer to top of statement tree to set stop bits for.
 \param post       Pointer to statement that comes just after the stopped statement.
 \param true_path  Set to TRUE if the current statement exists on the right of its parent.
 \param both       If TRUE, causes both false and true paths to set stop bits when next
                   statement is the post statement.

 Recursively traverses specified statement tree, setting the statement's stop bits
 that have either their next_true or next_false pointers pointing to the statement
 called post.
*/
void statement_set_stop( statement* stmt, statement* post, bool true_path, bool both ) {

  assert( stmt != NULL );

  if( ((stmt->next_true == post) && (stmt->next_false == post)) ||
      ((stmt->next_true == post) && (stmt->next_false == NULL)) ||
      (stmt->next_false == post) ) {
    if( true_path || both) {
      /* printf( "Setting STOP bit for statement %d\n", stmt->exp->id ); */
      stmt->exp->suppl.part.stmt_stop = 1;
    }
  } else {
    if( (stmt->next_true == stmt->next_false) ||
        (stmt->exp->suppl.part.stmt_connected == 0) ) {
      if( (stmt->next_true != NULL) && (stmt->next_true != post) ) { 
        statement_set_stop( stmt->next_true, post, TRUE, both );
      }
    } else {
      if( (stmt->next_true != NULL) && (stmt->next_true != post) ) {
        statement_set_stop( stmt->next_true, post, TRUE, both );
      }
      if( (stmt->next_false != NULL) && (stmt->next_false != post) ) {
        statement_set_stop( stmt->next_false, post, FALSE, both );
      }
    }
  }
  
  if( (stmt->next_true != NULL) && (stmt->next_false != NULL) ) {
    stmt->exp->suppl.part.stmt_connected = 0;
  }

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
int statement_get_last_line_helper( statement* stmt, statement* base ) {

  expression* last_exp;         /* Pointer to last expression in the statement tree */
  int         last_false = -1;  /* Last false path line number                      */ 
  int         last_true  = -1;  /* Last true path line number                       */

  if( stmt != NULL ) {

    /* Check out/traverse false path */
    if( (stmt->next_false == NULL) || (stmt->next_false == base) ) {
      last_exp   = expression_get_last_line_expr( stmt->exp );
      last_false = last_exp->line;
    } else if( ESUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0 ) {
      last_false = statement_get_last_line_helper( stmt->next_false, base );
    }

    /* Check out/traverse true path */
    if( (stmt->next_true == NULL) || (stmt->next_true == base) ) {
      last_exp  = expression_get_last_line_expr( stmt->exp );
      last_true = last_exp->line;
    } else if( ESUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0 ) {
      last_true = statement_get_last_line_helper( stmt->next_true, base );
    }

  }

  /* Return the greater of the two path last lines */
  return( (last_false > last_true) ? last_false : last_true );

}

int statement_get_last_line( statement* stmt ) {

  return( statement_get_last_line_helper( stmt, stmt ) );

}

void statement_remove_paths_helper( statement* curr, statement* start, statement* stmt ) {

  if( (curr != NULL) && (curr != start) ) {

    /* Handle TRUE path */
    if( curr->next_true == stmt ) {
      curr->next_true = NULL;
    } else {
      statement_remove_paths_helper( curr->next_true,  start, stmt );
    }

    /* Handle FALSE path */
    if( curr->next_false == stmt ) {
      curr->next_false = NULL;
    } else {
      statement_remove_paths_helper( curr->next_false, start, stmt );
    }

  }

}

void statement_remove_paths( statement* start, statement* stmt ) {

  if( start != NULL ) {

    /* Handle TRUE path */
    if( start->next_true == stmt ) {
      start->next_true = NULL;
    } else {
      statement_remove_paths_helper( start->next_true,  start, stmt );
    }

    /* Handle FALSE path */
    if( start->next_false == stmt ) {
      start->next_false = NULL;
    } else {
      statement_remove_paths_helper( start->next_false, start, stmt );
    }

  }

}

/*!
 \param curr   Pointer to current statement to deallocate.
 \param start  Pointer to statement of root of statement tree to deallocate.
 
 Recursively deallocates statements listed by the current statement tree.
 This function is called by the parser to remove statement trees that were
 created but found to have unsupported code in them.  This function takes
 care to not remove the same statement twice and to not infinitely loop.
*/
void statement_dealloc_recursive_helper( statement* curr, statement* start ) {
  
  if( (curr != NULL) && (curr != start) ) {
    
    /* Remove TRUE path */
    statement_dealloc_recursive_helper( curr->next_true,  start );
    
    /* Remove FALSE path */
    statement_dealloc_recursive_helper( curr->next_false, start );

    /* Disconnect statement from current module */
    db_remove_statement_from_current_module( curr );

    /* Set pointers to this statement to NULL */
    statement_remove_paths( start, curr );

    free_safe( curr );
    
  }
  
}

/*!
 \param stmt  Pointer to head of statement tree to deallocate.
 
 Recursively deallocates specified statement tree.
*/
void statement_dealloc_recursive( statement* stmt ) {
    
  if( stmt != NULL ) {
  
    /* Remove TRUE path */
    statement_dealloc_recursive_helper( stmt->next_true,  stmt );
  
    /* Remove FALSE path */
    statement_dealloc_recursive_helper( stmt->next_false, stmt );

    /* Disconnect statement from current module */
    db_remove_statement_from_current_module( stmt );

    /* Remove wait event signal list */
    sig_link_delete_list( stmt->wait_sig_head, FALSE );
  
    /* Set pointers to this statement to NULL */
    statement_remove_paths( stmt, stmt );

    free_safe( stmt );
    
  }
  
}

/*!
 \param stmt  Pointer to statement to deallocate.

 Deallocates specified statement from heap memory.  Does not
 remove attached expression (this is assumed to be cleaned up by the
 expression list removal function).
*/
void statement_dealloc( statement* stmt ) {

  if( stmt != NULL ) {
 
    /* Remove wait event signal list */
    sig_link_delete_list( stmt->wait_sig_head, FALSE );

    /* Finally, deallocate this statement */
    free_safe( stmt );

  }

}


/*
 $Log$
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

