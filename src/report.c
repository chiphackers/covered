/*!
 \file     report.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/29/2001
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "defines.h"
#include "report.h"
#include "util.h"
#include "line.h"
#include "toggle.h"
#include "comb.h"
#include "fsm.h"
#include "instance.h"
#include "stat.h"


/*!
 If set to a boolean value of TRUE, reports the line coverage for the specified database
 file; otherwise, omits line coverage from the report output.
*/
bool report_line       = TRUE;

/*!
 If set to a boolean value of TRUE, reports the toggle coverage for the specified database
 file; otherwise, omits toggle coverage from the report output.
*/
bool report_toggle     = TRUE;

/*!
 If set to a boolean value of TRUE, reports the combinational logic coverage for the specified
 database file; otherwise, omits combinational logic coverage from the report output.
*/
bool report_combination = TRUE;

/*!
 If set to a boolean value of TRUE, reports the finite state machine coverage for the
 specified database file; otherwise, omits finite state machine coverage from the
 report output.
*/
bool report_fsm        = FALSE;

/*!
 If set to a boolean value of TRUE, provides a verbose description of both the coverage
 and metric examples that were not covered.  If set to FALSE, provides a summarized
 coverage report containing only coverage percentage values.
*/
bool report_verbose    = FALSE;

/*!
 If set to a boolean value of TRUE, provides a coverage information for individual
 module instances.  If set to a value of FALSE, reports coverage information on a
 module basis, merging results from all instances of same module.
*/
bool report_instance   = FALSE;

char* output_file      = NULL;
char* input_db         = NULL;

extern mod_inst* instance_root;


/*!
 \param metrics  Specified metrics to calculate coverage for.

 Parses the specified string containing the metrics to test.  If
 a legal metric character is found, its corresponding flag is set
 to TRUE.  If a character is found that does not correspond to a
 metric, an error message is flagged to the user (a warning).
*/
void report_parse_metrics( char* metrics ) {

  char* ptr;          /* Pointer to current character being evaluated */
  char  msg[4096];    /* Warning message to specify to user           */

  /* Set all flags to FALSE */
  report_line        = FALSE;
  report_toggle      = FALSE;
  report_combination = FALSE;
  report_fsm         = FALSE;

  for( ptr=metrics; ptr<(metrics + strlen( metrics )); ptr++ ) {

    switch( *ptr ) {
      case 'l' :
      case 'L' :  report_line        = TRUE;  break;
      case 't' :
      case 'T' :  report_toggle      = TRUE;  break;
      case 'c' :
      case 'C' :  report_combination = TRUE;  break;
      case 'f' :
      case 'F' :  report_fsm         = TRUE;  break;
      default  :
        snprintf( msg, 4096, "Unknown metric specified '%c'...  Ignoring.", *ptr );
        print_output( msg, WARNING );
        break;
    }

  }

}

/*!
 \param argc  Number of arguments in argument list argv.
 \param argv  Argument list passed to this program.

 Parses the argument list for options.  If a legal option is
 found for the report command, the appropriate action is taken for
 that option.  If an option is found that is not allowed, an error
 message is reported to the user and the program terminates immediately.
*/
bool report_parse_args( int argc, char** argv ) {

  bool retval = TRUE;    /* Return value for this function */
  int  i;                /* Loop iterator                  */
  char err_msg[4096];    /* Error message for user         */

  i = 2;

  while( (i < argc) && retval ) {

    if( strncmp( "-m", argv[i], 2 ) == 0 ) {
    
      i++;
      report_parse_metrics( argv[i] );

    } else if( strncmp( "-v", argv[i], 2 ) == 0 ) {

      report_verbose = TRUE;

    } else if( strncmp( "-i", argv[i], 2 ) == 0 ) {

      report_instance = TRUE;

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      i++;
      if( is_directory( argv[i] ) ) {
        output_file = strdup( argv[i] );
      } else {
  	snprintf( err_msg, 4096, "Illegal output directory specified \"%s\"", argv[i] );
        print_output( err_msg, FATAL );
        retval = FALSE;
      }

    } else if( (i + 1) == argc ) {

      if( file_exists( argv[i] ) ) {
     
        input_db = strdup( argv[i] );
 
      } else {

        snprintf( err_msg, 4096, "Cannot find %s database file for opening", argv[i] );
        print_output( err_msg, FATAL );
        exit( 1 );

      }

    } else {

      snprintf( err_msg, 4096, "Unknown report command option \"%s\".  See \"covered -h\" for more information.", argv[i] );
      print_output( err_msg, FATAL );
      retval = FALSE;

    }

    i++;

  }

}

/*!
 \param root   Pointer to root of instance tree to search.
 
 Recursively parses instance tree, creating statistic structures for each
 of the instances in the tree.  Calculates summary coverage information for
 children nodes first and parent nodes after.  In this way, the parent nodes
 will have the accumulated information from themselves and all of their
 children.
*/
void report_gather_stats( mod_inst* root ) {

  mod_inst* curr;    /* Pointer to current instance being evaluated */

  /* Create a statistics structure for this instance */
  assert( root->stat == NULL );

  root->stat = statistic_create();

  /* Get coverage results for all children first */
  curr = root->child_head;
  while( curr != NULL ) {
    report_gather_stats( curr );
    assert( curr->stat != NULL );
    statistic_merge( root->stat, curr->stat );
    curr = curr->next;
  }

  /* Get coverage results for this instance */
  if( report_line ) {
    line_get_stats( root->mod->exp_head, &(root->stat->line_total), &(root->stat->line_hit) );
  }

  if( report_toggle ) {
    toggle_get_stats( root->mod->exp_head, 
                      root->mod->sig_head, 
                      &(root->stat->tog_total), 
                      &(root->stat->tog01_hit), 
                      &(root->stat->tog10_hit) );
  }

  if( report_combination ) {
    /* TBD */
  }

  if( report_fsm ) {
    /* TBD */
  }

}

/*!
 \param ofile    Pointer to output stream to display report information to.

 Generates generic report header for all reports.
*/
void report_print_header( FILE* ofile ) {

  if( report_verbose ) {
    fprintf( ofile, "Covered -- Verilog Coverage Detailed Report\n" );
    fprintf( ofile, "===========================================\n" );
  } else {
    fprintf( ofile, "Covered -- Verilog Coverage Summarized Report\n" );
    fprintf( ofile, "=============================================\n" );
  }

  fprintf( ofile, "\n" );

}

/*!
 \param ofile  Pointer to output stream to display report information to.

 Generates a coverage report based on the options specified on the command line
 to the specified output stream.
*/
void report_generate( FILE* ofile ) {

  report_print_header( ofile );

  /* If we are reporting on an instance, gather statistics first */
  if( report_instance ) {
    report_gather_stats( instance_root );
  }

  /* Call out the proper reports for the specified metrics to report */
  if( report_line ) {
    line_report( ofile, report_verbose, report_instance );
  }

  if( report_toggle ) {
    toggle_report( ofile, report_verbose, report_instance );
  }

  if( report_combination ) {
    combination_report( ofile, report_verbose, report_instance );
  }

  if( report_fsm ) {
    fsm_report( ofile, report_verbose, report_instance );
  }

}

/*!
 \param argc  Number of arguments in report command-line.
 \param argv  Arguments passed to report command to parse.
 \return Returns 0 is report is successful; otherwise, returns -1.

*/
int command_report( int argc, char** argv ) {

  int   retval = 0;    /* Return value of this function         */
  FILE* ofile;         /* Pointer to output stream              */
  FILE* dbfile;        /* Pointer to database file to read from */
  char  msg[4096];     /* Error message for user                */

  /* Initialize error suppression value */
  set_output_suppression( FALSE );

  /* Parse score command-line */
  report_parse_args( argc, argv );

  /* Open output stream */
  if( output_file != NULL ) {

    if( (ofile = fopen( output_file, "w" )) == NULL ) {

      snprintf( msg, 4096, "Unable to open %s for writing", output_file );
      print_output( msg, FATAL );
      exit( 1 );

    } else {

      /* Free up memory for holding output_file */
      free( output_file );

    }
    
  } else {
 
    ofile = stdout;

  }

  /* Open database file for reading */
  if( input_db == NULL ) {

    snprintf( msg, 4096, "Database file not specified in command line" );
    print_output( msg, FATAL );
    exit( 1 );

  } else {

    if( report_instance ) {
      db_read( input_db, READ_MODE_NO_MERGE );
    } else {
      db_read( input_db, READ_MODE_MOD_MERGE );
    }

  }

  report_generate( ofile );

  fclose( ofile );

  return( retval );

}

