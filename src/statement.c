/*!
 \file     statement.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     5/1/2002
*/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "defines.h"
#include "statement.h"
#include "expr.h"
#include "util.h"
#include "link.h"


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
  assert( stmt->next_false != NULL );

  /* Write succeeding statements first */
//  if( stmt->next_true != NULL ) {

  if( stmt->next_true != stmt->next_false ) {

    if( stmt->next_true != NULL ) {
      statement_db_write( stmt->next_true, ofile, scope );
    } else {
      statement_db_write( stmt->next_false, ofile, scope );
    }

  }

  assert( stmt->exp != NULL );

  /* Write out expression tree second */
  expression_db_write( stmt->exp, ofile, scope );

  /* Write out contents of this statement last */
  fprintf( ofile, "%d %d %s",
    DB_TYPE_STATEMENT,
    stmt->exp->id,
    scope
  );

  if( stmt->next_true != NULL ) {
    assert( stmt->next_true->exp != NULL );
    fprintf( ofile, " %d", stmt->next_true->exp->id );
  } else {
    fprintf( ofile, " 0" );
  }

  if( stmt->next_false != NULL ) {
    assert( stmt->next_false->exp != NULL );
    fprintf( ofile, " %d", stmt->next_false->exp->id );
  } else {
    fprintf( ofile, " 0" );
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

  bool       retval;         /* Return value of this function                                          */
  int        id;             /* ID of root expression that is associated with this statement           */
  char       modname[4096];  /* Scope of module to which this statement belongs                        */
  int        true_id;        /* ID of root expression that is associated with the next_true statement  */
  int        false_id;       /* ID of root expression that is associated with the next_false statement */
  expression tmpexp;         /* Temporary expression used for expression search                        */
  statement* stmt;           /* Pointer to newly created statement                                     */
  exp_link*  expl;           /* Pointer to found expression link                                       */
  stmt_link* stmtl;          /* Pointer to found statement link                                        */
  int        chars_read;     /* Number of characters read from line                                    */

  if( sscanf( *line, "%d %s %d %d%n", &id, modname, &true_id, &false_id, &chars_read ) == 6 ) {

    *line = *line + chars_read;

    if( (curr_mod == NULL) || (strcmp( curr_mod->name, modname ) != 0) ) {

      print_output( "Internal error:  statement in database written before its module", FATAL );
      retval = FALSE;

    } else {

      /* Find associated root expression */
      tmpexp.id = id;
      expl = exp_link_find( &tmpexp, curr_mod->exp_head );

      stmt = statement_create( expl->exp );

      /* We should always have a legit next_false statement */
      assert( false_id != 0 );

      if( true_id != false_id ) {

        /* Find next_false statement */
        stmtl = stmt_link_find( false_id, curr_mod->stmt_head );

        /* If stmtl NULL, indicates that FALSE statement written after current statement or not at all */
        assert( stmtl != NULL );

        stmt->next_false = stmtl->stmt;

        if( true_id != 0 ) {
   
          /* Find next_true statement */
          stmtl = stmt_link_find( true_id, curr_mod->stmt_head );

          /* If stmtl NULL, indicates that TRUE statement written after current statement or not at all */
          assert( stmtl != NULL );

          stmt->next_true = stmtl->stmt;

        }

      }

    }

  } else {

    print_output( "Unable to read statement value", FATAL );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param stmt  Pointer to statement sequence to connect ends for loopback.

 Traverses the specified stmt sequence (assumes that stmt is the statement at the
 top of the sequence).  When it reaches a statement that has both next_true and
 next_false set to NULL, sets next_true and next_false of that statement to point
 to the top of the sequence.  This will cause the statement to have a loopback
 effect.
*/
void statement_loopback( statement* stmt ) {

  statement* curr;     /* Pointer to current statement being evaluated */

  assert( stmt != NULL );

  curr = stmt;
  while( curr->next_false != NULL ) {
    curr = curr->next_false;
  }

  assert( curr->next_true == NULL );

  curr->next_true  = stmt;
  curr->next_false = stmt;

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

  if( (stmt != NULL) && (stmt->next_true != stmt->next_false) ) {

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
 
    /* Finally, deallocate this statement */
    free_safe( stmt );

  }

}


/* $Log$
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
