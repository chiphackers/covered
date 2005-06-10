/*!
 \file     race.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/15/2004

 \par
 The contents of this file contain functions to handle race condition checking and information
 handling regarding race conditions.  Since race conditions can cause the Covered simulator to not
 provide the same run-time order as the primary Verilog simulator, we need to be able to detect when
 race conditions are occurring within the design.  The conditions that are checked within the design
 during the parsing stage can be found \ref race_condition_types.

 \par
 The failure of any one of these rules will cause Covered to either display warning type messages to the user when
 the race condition checking flag has not been set or error messages when the race condition checking flag has
 been set by the user.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "race.h"
#include "db.h"
#include "util.h"
#include "vsignal.h"
#include "statement.h"
#include "iter.h"
#include "vector.h"
#include "link.h"


stmt_blk* sb = NULL;
int       sb_size;

/*!
 Tracks the number of race conditions that were detected during the race-condition checking portion of the
 scoring command.
*/
int races_found = 0;

/*!
 This array is used to output the various race condition violation messages in both the parsing and report functions
 */
const char* race_msgs[RACE_TYPE_NUM] = { "Sequential statement block contains blocking assignment(s)",
                                         "Combinational statement block contains non-blocking assignment(s)",
                                         "Mixed statement block contains blocking assignment(s)",
					 "Statement block contains both blocking and non-blocking assignment(s)", 
					 "Signal assigned in two different statement blocks",
					 "Signal assigned both in statement block and via input/inout port",
					 "System call $strobe used to output signal assigned via blocking assignment",
					 "Procedural assignment with #0 delay performed" };

extern int       flag_race_check;
extern char      user_msg[USER_MSG_LENGTH];
extern mod_link* mod_head;
extern module*   curr_module;


/*!
 \param reason      Numerical reason for why race condition was detected.
 \param start_line  Starting line of race condition block.
 \param end_line    Ending line of race condition block.

 \return Returns a pointer to the newly allocated race condition block.

 Allocates and initializes memory for a race condition block.
*/
race_blk* race_blk_create( int reason, int start_line, int end_line ) {

  race_blk* rb;  /* Pointer to newly allocated race condition block */

  rb             = (race_blk*)malloc_safe( sizeof( race_blk ), __FILE__, __LINE__ );
  rb->reason     = reason;
  rb->start_line = start_line;
  rb->end_line   = end_line;
  rb->next       = NULL;

  return( rb );

}

/*!
 \param mod   Pointer to module containing statement list to parse.
 \param expr  Pointer to root expression to find in statement list.

 \return Returns pointer to head statement of statement block containing the specified expression

 Finds the head statement of the statement block containing the expression specified in the parameter list.
 Verifies that the return value is never NULL (this would be an internal error if it existed).
*/
statement* race_get_head_statement( module* mod, expression* expr ) {

  stmt_iter  si;         /* Statement iterator                                     */
  statement* curr_stmt;  /* Pointer to current statement containing the expression */

  /* First, find the statement associated with this expression */
  while( (expr != NULL) && (expr->suppl.part.root == 0) ) {
    expr = expr->parent->expr;
  }
  curr_stmt = expr->parent->stmt;

  /* Second, find the head statement that contains the expression's statement */
  stmt_iter_reset( &si, mod->stmt_tail );

  while( (si.curr != NULL) && (si.curr->stmt != curr_stmt) ) {
    stmt_iter_next( &si );
  }

  assert( si.curr != NULL );

  /* Back up to the head statement once we have found the matching statement */
  stmt_iter_next( &si );
  stmt_iter_reverse( &si );
  stmt_iter_find_head( &si, FALSE );

  assert( si.curr != NULL );

  return( si.curr->stmt );

}

/*!
 \param stmt  Pointer to statement to search for in statement block array

 \return Returns index of found head statement in statement block array if found; otherwise,
         returns a value of -1 to indicate that no such statement block exists.
*/
int race_find_head_statement( statement* stmt ) {

  int i = 0;   /* Loop iterator                  */

  while( (i < sb_size) && (sb[i].stmt != stmt) ) {
    i++;
  }

  return( (i == sb_size) ? -1 : i );

}

void race_calc_stmt_blk_type( expression* expr, int sb_index ) {

  if( expr != NULL ) {

    /* Go to children to calculate further */
    race_calc_stmt_blk_type( expr->left,  sb_index );
    race_calc_stmt_blk_type( expr->right, sb_index );

    if( (expr->op == EXP_OP_PEDGE) || (expr->op == EXP_OP_NEDGE) ) {
      sb[sb_index].seq = TRUE;
    }

    if( expr->op == EXP_OP_AEDGE ) {
      sb[sb_index].cmb = TRUE;
    }

  }

}

void race_calc_expr_assignment( expression* exp, int sb_index ) {

  switch( exp->op ) {
    case EXP_OP_ASSIGN  :
    case EXP_OP_BASSIGN :  sb[sb_index].bassign = TRUE;  break;
    case EXP_OP_NASSIGN :  sb[sb_index].nassign = TRUE;  break;
    default             :  break;
  }

}

void race_calc_assignments_helper( statement* stmt, statement* head, int sb_index ) {

  if( stmt != NULL ) {
	
    /* Calculate children statements */
    if( (stmt != head) && (ESUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0) ) {
      race_calc_assignments_helper( stmt->next_true, head, sb_index );
      race_calc_assignments_helper( stmt->next_false, head, sb_index );
    }

    /* Calculate assignment operator type */
    race_calc_expr_assignment( stmt->exp, sb_index );

  }

}

void race_calc_assignments( int sb_index ) {

  /* Calculate head statement assignment type */
  race_calc_expr_assignment( sb[sb_index].stmt->exp, sb_index );

  /* Calculate children statements */
  race_calc_assignments_helper( sb[sb_index].stmt->next_true,  sb[sb_index].stmt, sb_index );
  race_calc_assignments_helper( sb[sb_index].stmt->next_false, sb[sb_index].stmt, sb_index );

}

/*!
 \param expr    Pointer to expression containing signal that was found to be in a race condition.
 \param mod     Pointer to module containing detected race condition
 \param stmt    Pointer to expr's head statement
 \param base    Pointer to head statement block that was found to be in conflict with stmt
 \param reason  Specifies what type of race condition was being checked that failed

 Outputs necessary information to user regarding the race condition that was detected and performs any
 necessary memory cleanup to remove the statement block involved in the race condition.
*/
void race_handle_race_condition( expression* expr, module* mod, statement* stmt, statement* base, int reason ) {

  race_blk* rb;  /* Pointer to race condition block to add to specified module */
  int       i;   /* Loop iterator                                              */

  /* If the base pointer is NULL, the stmt refers to a statement block that conflicts with an input port */
  if( base == NULL ) {

    if( flag_race_check != NORMAL ) {

      print_output( "", (flag_race_check + 1), __FILE__, __LINE__ );
      snprintf( user_msg, USER_MSG_LENGTH, "Possible race condition detected - %s", race_msgs[reason] );
      print_output( user_msg, flag_race_check, __FILE__, __LINE__ );
      snprintf( user_msg, USER_MSG_LENGTH, "  Signal assigned in file: %s, line: %d", mod->filename, expr->line );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );

      if( flag_race_check == WARNING ) {
        print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
        snprintf( user_msg, USER_MSG_LENGTH, "    Statement block starting at file: %s, line: %d", mod->filename, stmt->exp->line );
        print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
      }
              
    }

  /* If the stmt and base pointers are pointing to different statements, we will output conflict for stmt */
  } else if( stmt != base ) { 

    if( flag_race_check != NORMAL ) {

      print_output( "", (flag_race_check + 1), __FILE__, __LINE__ );
      snprintf( user_msg, USER_MSG_LENGTH, "Possible race condition detected - %s", race_msgs[reason] );
      print_output( user_msg, flag_race_check, __FILE__, __LINE__ );
      snprintf( user_msg, USER_MSG_LENGTH, "  Signal assigned in file: %s, line: %d", mod->filename, expr->line );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );
      snprintf( user_msg, USER_MSG_LENGTH, "  Signal also assigned in statement starting at file: %s, line: %d", mod->filename, base->exp->line );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );

      if( flag_race_check == WARNING ) {
        print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
        snprintf( user_msg, USER_MSG_LENGTH, "    Statement block starting at file: %s, line: %d", mod->filename, stmt->exp->line );
        print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
      }

    }

  /* If stmt and base are pointing to the same statement, just report that we are removing the base statement */
  } else {

    if( flag_race_check != NORMAL ) {

      if( reason != 6 ) {
	
        print_output( "", (flag_race_check + 1), __FILE__, __LINE__ );
	snprintf( user_msg, USER_MSG_LENGTH, "Possible race condition detected - %s", race_msgs[reason] );
        print_output( user_msg, flag_race_check, __FILE__, __LINE__ );
        snprintf( user_msg, USER_MSG_LENGTH, "  Statement block starting in file: %s, line: %d", mod->filename, stmt->exp->line );
        print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );
	if( flag_race_check == WARNING ) {
	  print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
	}

      } else {

	if( flag_race_check == WARNING ) {
          print_output( "", WARNING_WRAP, __FILE__, __LINE__ );
	  print_output( "* Safely removing statement block from coverage consideration", WARNING, __FILE__, __LINE__ );
          snprintf( user_msg, USER_MSG_LENGTH, "  Statement block starting at file: %s, line: %d", mod->filename, stmt->exp->line );
          print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
	}

      }

    }

  }

  /* Create a race condition block and add it to current module */
  rb = race_blk_create( reason, stmt->exp->line, statement_get_last_line( stmt ) );

  /* Add the newly created race condition block to the current module */
  if( mod->race_head == NULL ) {
    mod->race_head = mod->race_tail = rb;
  } else {
    mod->race_tail->next = rb;
    mod->race_tail       = rb;
  }

  /* Set remove flag in stmt_blk array to remove this module from memory */
  i = race_find_head_statement( stmt );
  assert( i != -1 );
  sb[i].remove = TRUE;

  /* Increment races found flag */
  races_found++;

}

void race_check_assignment_types( module* mod ) {

  int i;  /* Loop iterator */

  for( i=0; i<sb_size; i++ ) {

    /* Check that a sequential logic block contains only non-blocking assignments */
    if( sb[i].seq && !sb[i].cmb && sb[i].bassign ) {

      race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_SEQ_USES_NON_BLOCK );

    /* Check that a combinational logic block contains only blocking assignments */
    } else if( !sb[i].seq && sb[i].cmb && sb[i].nassign ) {

      race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_CMB_USES_BLOCK );

    /* Check that mixed logic block contains only non-blocking assignments */
    } else if( sb[i].seq && sb[i].cmb && sb[i].bassign ) {

      race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_MIX_USES_NON_BLOCK );

    /* Check that a statement block doesn't contain both blocking and non-blocking assignments */
    } else if( sb[i].bassign && sb[i].nassign ) {

      race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_HOMOGENOUS );

    }

  }

}

void race_check_one_block_assignment( module* mod ) {

  sig_link*  sigl;                /* Pointer to current signal                                          */
  exp_link*  expl;                /* Pointer to current expression                                      */
  statement* curr_stmt;           /* Pointer to current statement                                       */
  int        sig_stmt;            /* Index of base signal statement in statement block array            */
  bool       race_found = FALSE;  /* Specifies if at least one race condition was found for this signal */
  bool       curr_race;           /* Set to TRUE if race condition was found in current iteration       */

  sigl = mod->sig_head;
  while( sigl != NULL ) {

    sig_stmt = -1;

    /* Iterate through expressions */
    expl = sigl->sig->exp_head;
    while( expl != NULL ) {
					      
      /* Only look at expressions that are part of LHS */
      if( expl->exp->suppl.part.lhs == 1 ) {      

	/*
	 If the signal was a part select, set the appropriate misc bits to indicate what
	 bits have been assigned.
        */
	switch( expl->exp->op ) {
          case EXP_OP_SIG :
            curr_race = vsignal_set_assigned( sigl->sig, ((sigl->sig->value->width - 1) + sigl->sig->lsb), sigl->sig->lsb );
	    break;
          case EXP_OP_SBIT_SEL :
            if( expl->exp->left->op == EXP_OP_STATIC ) {
              curr_race = vsignal_set_assigned( sigl->sig, vector_to_int( expl->exp->left->value ), vector_to_int( expl->exp->left->value ) );
	    } else { 
              curr_race = vsignal_set_assigned( sigl->sig, ((sigl->sig->value->width - 1) + sigl->sig->lsb), sigl->sig->lsb );
            }
	    break;
          case EXP_OP_MBIT_SEL :
	    if( (expl->exp->left->op == EXP_OP_STATIC) && (expl->exp->right->op == EXP_OP_STATIC) ) {
              curr_race = vsignal_set_assigned( sigl->sig, vector_to_int( expl->exp->left->value ), vector_to_int( expl->exp->right->value ) );
            } else {
              curr_race = vsignal_set_assigned( sigl->sig, ((sigl->sig->value->width - 1) + sigl->sig->lsb), sigl->sig->lsb );
            }
	    break;
          default :
            curr_race = FALSE;
	    break;	
        }
          
        /* Get expression's head statement */
        curr_stmt = race_get_head_statement( mod, expl->exp );

        /* Check to see if the current signal is already being assigned in another statement */
        if( sig_stmt == -1 ) {

	  /* Get index of base signal statement in sb array */
          sig_stmt = race_find_head_statement( curr_stmt );
	  assert( sig_stmt != -1 );

          /* Check to see if current signal is also an input port */ 
          if( (sigl->sig->value->suppl.part.inport == 1) || curr_race ) {
            race_handle_race_condition( expl->exp, mod, curr_stmt, NULL, RACE_TYPE_ASSIGN_IN_ONE_BLOCK2 );
	    sb[sig_stmt].remove = TRUE;
          }

        } else if( (sb[sig_stmt].stmt != curr_stmt) && curr_race ) {

          race_handle_race_condition( expl->exp, mod, curr_stmt, sb[sig_stmt].stmt, RACE_TYPE_ASSIGN_IN_ONE_BLOCK1 );
	  sb[sig_stmt].remove = TRUE;
	  race_found = TRUE;

        }

      }

      expl = expl->next;

    }

    sigl = sigl->next;

  }

}

/*!
 \return Returns TRUE if no race conditions were found or the user specified that we should continue
         to score the design.
*/
void race_check_race_count() {

  /*
   If we were able to find race conditions and the user specified to check for race conditions
   and quit, display the number of race conditions found and return FALSE to cause everything to
   halt.
  */
  if( (races_found > 0) && (flag_race_check == FATAL) ) {

    snprintf( user_msg, USER_MSG_LENGTH, "%d race conditions were detected.  Exiting score command.", races_found );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );

  }

}

/*!
 \param Returns TRUE if calling function should continue with scoring.

 Performs race checking for the currently loaded module.  This function should be called when
 the endmodule keyword is detected in the current module.
*/
void race_check_modules() {

  int        sb_index;  /* Index to statement block array              */
  stmt_iter  si;        /* Statement iterator                          */
  mod_link*  modl;      /* Pointer to current module link              */
  int        i;         /* Loop iterators                              */

  modl = mod_head;

  while( modl != NULL ) {

    /* Clear statement block array size */
    sb_size = 0;

    /* First, get the size of the statement block array */
    stmt_iter_reset( &si, modl->mod->stmt_tail );
    while( si.curr != NULL ) {
      if( si.curr->stmt->exp->suppl.part.stmt_head == 1 ) {
        sb_size++;
      }
      stmt_iter_next( &si );
    }

    if( sb_size > 0 ) {

      /* Allocate memory for the statement block array and clear current index */
      sb       = (stmt_blk*)malloc_safe( (sizeof( stmt_blk ) * sb_size), __FILE__, __LINE__ );
      sb_index = 0;

      /* Second, populate the statement block array with pointers to the head statements */
      stmt_iter_reset( &si, modl->mod->stmt_tail );
      while( si.curr != NULL ) {
        if( si.curr->stmt->exp->suppl.part.stmt_head == 1 ) {
          sb[sb_index].stmt    = si.curr->stmt;
          sb[sb_index].remove  = FALSE;
	  sb[sb_index].seq     = FALSE;
	  sb[sb_index].cmb     = FALSE;
	  sb[sb_index].bassign = FALSE;
	  sb[sb_index].nassign = FALSE;
	  race_calc_stmt_blk_type( sb[sb_index].stmt->exp, sb_index );
	  race_calc_assignments( sb_index );
          sb_index++; 
        }
        stmt_iter_next( &si );
      }

      /* Perform checks #1 - #5 */
      race_check_assignment_types( modl->mod );

      /* Perform check #6 */
      race_check_one_block_assignment( modl->mod );

      /* Cleanup statements to be removed */
      stmt_iter_reverse( &si );
      curr_module = modl->mod;
      for( i=0; i<sb_size; i++ ) {
        if( sb[i].remove ) {
          db_remove_statement( sb[i].stmt );
        }
      }

      /* Deallocate stmt_blk list */
      free_safe( sb );

    }

    modl = modl->next;

  }

  /* Handle output if any race conditions were found */
  race_check_race_count();

}

/*!
 \param rb    Pointer to race condition block to write to specified output file
 \param file  File handle of output stream to write.

 \return Returns TRUE if write occurred sucessfully; otherwise, returns FALSE.

 Writes contents of specified race condition block to the specified output stream.
*/
bool race_db_write( race_blk* rb, FILE* file ) {

  bool retval = TRUE;  /* Return value for this function */

  fprintf( file, "%d %d %d %d\n",
    DB_TYPE_RACE,
    rb->reason,
    rb->start_line,
    rb->end_line
  );

  return( retval );

}

/*!
 \param line      Pointer to line containing information for a race condition block.
 \param curr_mod  Pointer to current module to store race condition block to.

 \return Returns TRUE if line was read successfully; otherwise, returns FALSE.

 Reads the specified line from the CDD file and parses it for race condition block
 information.
*/
bool race_db_read( char** line, module* curr_mod ) {

  bool      retval = TRUE;  /* Return value for this function                 */
  int       start_line;     /* Starting line for race condition block         */
  int       end_line;       /* Ending line for race condition block           */
  int       reason;         /* Reason for why the race condition block exists */
  int       chars_read;     /* Number of characters read via sscanf           */
  race_blk* rb;             /* Pointer to newly created race condition block  */

  if( sscanf( *line, "%d %d %d%n", &reason, &start_line, &end_line, &chars_read ) == 3 ) {

    *line = *line + chars_read;

    /* Create the new race condition block */
    rb = race_blk_create( reason, start_line, end_line );

    /* Add the new race condition block to the current module */
    if( curr_mod->race_head == NULL ) {
      curr_mod->race_head = curr_mod->race_tail = rb;
    } else {
      curr_mod->race_tail->next = rb;
      curr_mod->race_tail       = rb;
    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param curr        Pointer to head of race condition block list.
 \param race_total  Pointer to value that will hold the total number of race conditions in this module.
 \param type_total  Pointer to array containing number of race conditions found for each violation type.

 Iterates through specified race condition block list, totaling the number of race conditions found as
 well as tallying each type of race condition.
*/
void race_get_stats( race_blk* curr, int* race_total, int type_total[][RACE_TYPE_NUM] ) {

  int i;  /* Loop iterator */

  /* Clear totals */
  *race_total = 0;
  for( i=0; i<RACE_TYPE_NUM; i++ ) {
    (*type_total)[i] = 0;
  }

  /* Tally totals */
  while( curr != NULL ) {
    (*type_total)[curr->reason]++;
    (*race_total)++;
    curr = curr->next;
  }

}

bool race_report_summary( FILE* ofile, mod_link* head ) {

  bool found = FALSE;  /* Return value for this function */

  while( head != NULL ) {

    found = (head->mod->stat->race_total > 0) ? TRUE : found;

    fprintf( ofile, "  %-20.20s    %-20.20s        %d\n", 
             head->mod->name,
	     get_basename( head->mod->filename ),
	     head->mod->stat->race_total );

    head = head->next;

  }

  return( found );

}

void race_report_verbose( FILE* ofile, mod_link* head ) {

  race_blk* curr_race;  /* Pointer to current race condition block */

  while( head != NULL ) {

    if( head->mod->stat->race_total > 0 ) {

      fprintf( ofile, "\n" );
      fprintf( ofile, "    Module: %s, File: %s\n", 
               head->mod->name, 
               head->mod->filename );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      fprintf( ofile, "      Starting Line #     Race Condition Violation Reason\n" );
      fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );

      curr_race = head->mod->race_head;
      while( curr_race != NULL ) {
        fprintf( ofile, "              %7d:    %s\n", curr_race->start_line, race_msgs[curr_race->reason] );
	curr_race = curr_race->next;
      }

      fprintf( ofile, "\n" );

    }

    head = head->next;
											       
  }

}

/*!
 \param ofile    Output stream to display report information to.
 \param verbose  Specifies if summary or verbose output should be displayed.

 Generates the race condition report information and displays it to the specified
 output stream.
*/
void race_report( FILE* ofile, bool verbose ) {

  bool found;  /* If set to TRUE, race conditions were found */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   RACE CONDITION VIOLATIONS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "Module                    Filename                 Number of Violations found\n" );
  fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

  found = race_report_summary( ofile, mod_head );

  if( verbose && found ) {
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    race_report_verbose( ofile, mod_head );
  }

  fprintf( ofile, "\n\n" );

}

/*!
 \param modname   Name of module to search for
 \param lines     Pointer to an array of lines that contain line numbers of race condition statements
 \param reasons   Pointer to an array of race condition reason integers, one for each line in the lines array
 \param line_cnt  Pointer to number of elements that exist in lines array

 \return Returns TRUE if the specified module name was found in the design; otherwise, returns FALSE.

 Collects all of the line numbers in the specified module that were ignored from coverage due to
 detecting a race condition.  This function is primarily used by the GUI for outputting purposes.
*/
bool race_collect_lines( char* modname, int** lines, int** reasons, int* line_cnt ) {

  bool      retval    = TRUE;  /* Return value for this function                           */
  module    mod;               /* Temporary module used to search for module name          */
  mod_link* modl;              /* Pointer to found module link containing specified module */
  race_blk* curr_race = NULL;  /* Pointer to current race condition block                  */
  int       i;                 /* Loop iterator                                            */
  int       line_size = 20;    /* Current number of lines allocated in lines array         */

  mod.name = strdup_safe( modname, __FILE__, __LINE__ );

  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {

    /* Begin by allocating some memory for the lines */
    *lines    = (int*)malloc_safe( (sizeof( int ) * line_size), __FILE__, __LINE__ );
    *reasons  = (int*)malloc_safe( (sizeof( int ) * line_size), __FILE__, __LINE__ );
    *line_cnt = 0;

    curr_race = modl->mod->race_head;
    while( curr_race != NULL ) {
      for( i=curr_race->start_line; i<=curr_race->end_line; i++ ) {
	if( *line_cnt == line_size ) {
          line_size += 20;
	  *lines   = (int*)realloc( *lines, (sizeof( int ) * line_size) );
	  *reasons = (int*)realloc( *lines, (sizeof( int ) * line_size) );
	}
        (*lines)[*line_cnt]   = i;
        (*reasons)[*line_cnt] = curr_race->reason;
	(*line_cnt)++;
      }
      curr_race = curr_race->next;
    }

  } else { 

    retval = FALSE;

  }

  free_safe( mod.name );

  return( retval );

}

/*!
 \param rb  Pointer to race condition block to deallocate.

 Recursively deallocates the specified race condition block list.
*/
void race_blk_delete_list( race_blk* rb ) {

  if( rb != NULL ) {

    /* Delete the next race condition block first */
    race_blk_delete_list( rb->next );

    /* Deallocate the memory for this race condition block */
    free_safe( rb );

  }

}

/*
 $Log$
 Revision 1.23  2005/02/07 22:19:46  phase1geo
 Added code to output race condition reasons to informational bar.  Also added code to
 output toggle and combinational logic output to information bar when cursor is over
 an expression that, when clicked on, will take you to the detailed coverage window.

 Revision 1.22  2005/02/05 06:20:58  phase1geo
 Added ascii report output for race conditions.  There is a segmentation fault
 bug associated with instance reporting.  Need to look into further.

 Revision 1.21  2005/02/05 05:29:25  phase1geo
 Added race condition reporting to GUI.

 Revision 1.20  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.19  2005/02/04 23:55:53  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.18  2005/02/03 05:48:33  phase1geo
 Fixing bugs in race condition checker.  Adding race2.1 diagnostic.  Regression
 currently has some failures due to these changes.

 Revision 1.17  2005/02/03 04:59:13  phase1geo
 Fixing bugs in race condition checker.  Updated regression.

 Revision 1.16  2005/02/01 05:11:18  phase1geo
 Updates to race condition checker to find blocking/non-blocking assignments in
 statement block.  Regression still runs clean.

 Revision 1.15  2005/01/27 13:33:50  phase1geo
 Added code to calculate if statement block is sequential, combinational, both
 or none.

 Revision 1.14  2005/01/25 13:42:27  phase1geo
 Fixing segmentation fault problem with race condition checking.  Added race1.1
 to regression.  Removed unnecessary output statements from previous debugging
 checkins.

 Revision 1.13  2005/01/11 14:24:16  phase1geo
 Intermediate checkin.

 Revision 1.12  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.11  2005/01/10 13:44:58  phase1geo
 Fixing case where signal selects are being assigned by different statements that
 do not overlap.  We do not have race conditions in this case.

 Revision 1.10  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.9  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.8  2005/01/04 14:37:00  phase1geo
 New changes for race condition checking.  Things are uncompilable at this
 point.

 Revision 1.7  2004/12/20 04:12:00  phase1geo
 A bit more race condition checking code added.  Still not there yet.

 Revision 1.6  2004/12/18 16:23:18  phase1geo
 More race condition checking updates.

 Revision 1.5  2004/12/17 22:29:35  phase1geo
 More code added to race condition feature.  Still not usable.

 Revision 1.4  2004/12/17 14:27:46  phase1geo
 More code added to race condition checker.  This is in an unusable state at
 this time.

 Revision 1.3  2004/12/16 23:31:48  phase1geo
 More work done on race condition code.

 Revision 1.2  2004/12/16 15:22:01  phase1geo
 Adding race.c compilation to Makefile and adding documentation to race.c.

*/

