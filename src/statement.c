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
 supplemental value that are only used by expressions:  #SUPPL_LSB_STMT_HEAD and
 #SUPPL_LSB_STMT_STOP.

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

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "defines.h"
#include "statement.h"
#include "expr.h"
#include "util.h"
#include "link.h"
#include "sim.h"


/*!
 \param exp   Pointer to root expression of expression tree for this statement.
  
 \return Returns pointer to the newly created statement.

 Creates a new statement structure from heap memory and initializes it with the
 specified parameter information.
*/
statement* statement_create( expression* exp ) {

  statement* stmt;   /* Pointer to newly created statement */

  stmt                    = (statement*)malloc_safe( sizeof( statement ) );
  stmt->exp               = exp;
  stmt->exp->parent->stmt = stmt;
  stmt->exp->suppl        = stmt->exp->suppl | (0x1 << SUPPL_LSB_ROOT);
  stmt->next_true         = NULL;
  stmt->next_false        = NULL;

  return( stmt );

}

/*!
 \param stmt   Pointer to statement to write out value.
 \param ofile  Pointer to output file to write statement line to.
 \param scope  Scope of parent module which contains the specified statement.

 Recursively writes the contents of the specified statement tree (and its
 associated expression trees to the specified output stream.
*/
void statement_db_write( statement* stmt, FILE* ofile, char* scope ) {

  assert( stmt != NULL );

  printf( "In statement_db_write, id: %d\n", stmt->exp->id );

#ifdef EFFICIENCY_CODE
  /* Write succeeding statements first */
  if( SUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0 ) {

    statement_db_write( stmt->next_true, ofile, scope );
    statement_db_write( stmt->next_false, ofile, scope );

  }

  assert( stmt->exp != NULL );

  /* Write out expression tree second */
  expression_db_write( stmt->exp, ofile, scope );
#endif

  /* Write out contents of this statement last */
  fprintf( ofile, "%d %d %s",
    DB_TYPE_STATEMENT,
    stmt->exp->id,
    scope
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
 
 \return Returns TRUE if the line is read without error; otherwise, returns FALSE.

 Reads in the contents of the statement from the specified line, creates
 a statement structure to hold the contents.
*/
bool statement_db_read( char** line, module* curr_mod ) {

  bool       retval = TRUE;  /* Return value of this function                                          */
  int        id;             /* ID of root expression that is associated with this statement           */
  char       modname[4096];  /* Scope of module to which this statement belongs                        */
  int        true_id;        /* ID of root expression that is associated with the next_true statement  */
  int        false_id;       /* ID of root expression that is associated with the next_false statement */
  expression tmpexp;         /* Temporary expression used for expression search                        */
  statement* stmt;           /* Pointer to newly created statement                                     */
  exp_link*  expl;           /* Pointer to found expression link                                       */
  stmt_link* stmtl;          /* Pointer to found statement link                                        */
  int        chars_read;     /* Number of characters read from line                                    */

  if( sscanf( *line, "%d %s %d %d%n", &id, modname, &true_id, &false_id, &chars_read ) == 4 ) {

    *line = *line + chars_read;

    if( (curr_mod == NULL) || (strcmp( curr_mod->name, modname ) != 0) ) {

      print_output( "Internal error:  statement in database written before its module", FATAL );
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
        assert( stmtl != NULL );
        stmt->next_true = stmtl->stmt;
      }

      /* Find and link next_false */
      if( false_id == id ) {
        stmt->next_false = stmt;
      } else if( false_id != 0 ) {
        stmtl = stmt_link_find( false_id, curr_mod->stmt_head );
        assert( stmtl != NULL );
        stmt->next_false = stmtl->stmt;
      }

      /* Add statement to module statement list */
      stmt_link_add_tail( stmt, &(curr_mod->stmt_head), &(curr_mod->stmt_tail) );

//      stmt_link_display( curr_mod->stmt_head );

      /* Possibly add statement to presimulation queue */
      sim_add_stmt_to_queue( stmt );

    }

  } else {

    print_output( "Unable to read statement value", FATAL );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param curr_stmt  Pointer to statement sequence to traverse.
 \param next_stmt  Pointer to statement to connect ends to.
 \param set_stop   If TRUE and the next_true and next_false are NULL, set stop bit.

 Recursively traverses the specified stmt sequence.  When it reaches a statement 
 that has either next_true or next_false set to NULL, sets next_true and/or 
 next_false of that statement to point to the next_stmt statement.
*/
void statement_connect( statement* curr_stmt, statement* next_stmt, bool set_stop ) {

  bool allow_stop;      /* If TRUE, sets the set_stop value for TRUE paths */
  static int count = 0;

  assert( curr_stmt != NULL );
  assert( next_stmt != NULL );

  if( count == 100 ) {
    assert( count == 0 );
  } else {
    count++;
  }

  /* Set STOP bit if necessary */
  if( (curr_stmt->next_true == NULL) && (curr_stmt->next_false == NULL) && set_stop ) {

    printf( "Setting STOP bit for statement %d\n", curr_stmt->exp->id );
    curr_stmt->exp->suppl = curr_stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_STOP);

  }

  /* Traverse TRUE path */
  if( curr_stmt->next_true == NULL ) {

    printf( "curr: %d, next: %d, Setting TRUE\n", curr_stmt->exp->id, next_stmt->exp->id );
    curr_stmt->next_true = next_stmt;

  } else {

    if( curr_stmt->next_true != next_stmt ) {
      printf( "curr: %d, next: %d, Traversing TRUE path\n", curr_stmt->exp->id, next_stmt->exp->id );
      allow_stop = ((curr_stmt->next_true != curr_stmt->next_false) &&
                    (SUPPL_OP( curr_stmt->exp->suppl ) != EXP_OP_DELAY));
      statement_connect( curr_stmt->next_true, next_stmt, allow_stop );
    }

  }

  /* Traverse FALSE path */
  if( curr_stmt->next_false == NULL ) {

    if( SUPPL_OP( curr_stmt->exp->suppl ) != EXP_OP_DELAY ) {
      printf( "curr: %d, next: %d, Setting FALSE\n", curr_stmt->exp->id, next_stmt->exp->id );
      curr_stmt->next_false = next_stmt;
    }

  } else {

    if( curr_stmt->next_false != next_stmt ) {
      printf( "curr: %d, next: %d, Traversing FALSE path\n", curr_stmt->exp->id, next_stmt->exp->id );
      statement_connect( curr_stmt->next_false, next_stmt, FALSE );
    }

  }

}

/*!
 \param stmt  Pointer to statement to deallocate.

 Deallocates specified statement from heap memory.  First remove expression
 tree of this statement.  Second, remove all statements following this
 statement.  Finally, remove the specified statement itself.  Since statements
 can be circular in nature, we will not remove a statement that has its
 expression tree already deallocated.  This will prevent this function from
 getting into an infinite loop (or stack overflow since this function is
 recursive).
*/
void statement_dealloc( statement* stmt ) {

  if( stmt != NULL ) {

#ifdef EFFICIENCY_CODE
    assert( stmt->exp != NULL );

    /* Deallocate entire expression tree */
    expression_dealloc( stmt->exp, FALSE );
    stmt->exp = NULL;

    if( stmt->next_true != NULL ) {
      statement_dealloc( stmt->next_true );
      stmt->next_true = FALSE;
    }

    if( stmt->next_false != NULL ) {
      statement_dealloc( stmt->next_false );
      stmt->next_false = FALSE;
    }
#endif
 
    /* Finally, deallocate this statement */
    free_safe( stmt );

  }

}


/* $Log$
/* Revision 1.15  2002/06/27 12:36:47  phase1geo
/* Fixing bugs with scoring.  I think I got it this time.
/*
/* Revision 1.14  2002/06/26 22:09:17  phase1geo
/* Removing unecessary output and updating regression Makefile.
/*
/* Revision 1.13  2002/06/26 03:45:48  phase1geo
/* Fixing more bugs in simulator and report functions.  About to add support
/* for delay statements.
/*
/* Revision 1.12  2002/06/25 21:46:10  phase1geo
/* Fixes to simulator and reporting.  Still some bugs here.
/*
/* Revision 1.11  2002/06/25 12:48:38  phase1geo
/* Fixing case where statement's true and false paths point to itself when
/* reading in CDD.
/*
/* Revision 1.10  2002/06/25 03:39:03  phase1geo
/* Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
/* Fixed some report bugs though there are still some remaining.
/*
/* Revision 1.9  2002/06/25 02:02:04  phase1geo
/* Fixing bugs with writing/reading statements and with parsing design with
/* statements.  We now get to the scoring section.  Some problems here at
/* the moment with the simulator.
/*
/* Revision 1.7  2002/06/24 04:54:48  phase1geo
/* More fixes and code additions to make statements work properly.  Still not
/* there at this point.
/*
/* Revision 1.6  2002/06/23 21:18:22  phase1geo
/* Added appropriate statement support in parser.  All parts should be in place
/* and ready to start testing.
/*
/* Revision 1.5  2002/06/22 05:27:30  phase1geo
/* Additional supporting code for simulation engine and statement support in
/* parser.
/*
/* Revision 1.4  2002/06/21 05:55:05  phase1geo
/* Getting some codes ready for writing simulation engine.  We should be set
/* now.
/*
/* Revision 1.3  2002/05/13 03:02:58  phase1geo
/* Adding lines back to expressions and removing them from statements (since the line
/* number range of an expression can be calculated by looking at the expression line
/* numbers).
/*
/* Revision 1.2  2002/05/03 03:39:36  phase1geo
/* Removing all syntax errors due to addition of statements.  Added more statement
/* support code.  Still have a ways to go before we can try anything.  Removed lines
/* from expressions though we may want to consider putting these back for reporting
/* purposes.
/*
/* Revision 1.1  2002/05/02 03:27:42  phase1geo
/* Initial creation of statement structure and manipulation files.  Internals are
/* still in a chaotic state.
/* */
