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
 \file     report.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/29/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_TCLTK
#include <tcl.h>
#include <tk.h>
#endif

#include "assertion.h"
#include "binding.h"
#include "comb.h"
#include "db.h"
#include "defines.h"
#include "fsm.h"
#include "info.h"
#include "instance.h"
#include "line.h"
#include "memory.h"
#include "ovl.h"
#include "race.h"
#include "report.h"
#include "stat.h"
#include "tcl_funcs.h"
#include "toggle.h"
#include "util.h"


extern char         user_msg[USER_MSG_LENGTH];
extern db**         db_list;
extern unsigned int db_size;
extern unsigned int curr_db;
extern str_link*    merge_in_head;
extern str_link*    merge_in_tail;
extern isuppl       info_suppl;
extern char*        cdd_message;

/*!
 If set to a boolean value of TRUE, reports the line coverage for the specified database
 file; otherwise, omits line coverage from the report output.
*/
bool report_line = TRUE;

/*!
 If set to a boolean value of TRUE, reports the toggle coverage for the specified database
 file; otherwise, omits toggle coverage from the report output.
*/
bool report_toggle = TRUE;

/*!
 If set to a boolean value of TRUE, reports the combinational logic coverage for the specified
 database file; otherwise, omits combinational logic coverage from the report output.
*/
bool report_combination = TRUE;

/*!
 If set to a boolean value of TRUE, reports the logic event coverage for the specified database file;
 otherwise, omits logic event coverage from the report output.
*/
bool report_event = TRUE;

/*!
 If set to a boolean value of TRUE, reports the finite state machine coverage for the
 specified database file; otherwise, omits finite state machine coverage from the
 report output.
*/
bool report_fsm = TRUE;

/*!
 If set to a boolean value of TRUE, reports the race condition violations for the specified
 database file; otherwise, omits this information from the report output.
*/
bool report_race = FALSE;

/*!
 If set to a boolean value of TRUE, reports the assertion coverage for the specified
 database file; otherwise, omits assertion coverage from the report output.
*/
bool report_assertion = FALSE;

/*!
 If set to a boolean value of TRUE, reports the memory coverage for the specified database file;
 otherwise, omits memory coverage from the report output.
*/
bool report_memory = FALSE;

/*!
 If set to a boolean value of TRUE, provides a coverage information for individual
 functional unit instances.  If set to a value of FALSE, reports coverage information on a
 functional unit basis, merging results from all instances of same functional unit.
*/
bool report_instance = FALSE;

/*!
 If set to a boolean value of TRUE, displays covered logic for a particular CDD file.
 By default, Covered will display uncovered logic.  Must be used in conjunction with
 the -d v|d (verbose output) option.
*/
bool report_covered = FALSE;

/*!
 If set to a boolean value of TRUE, displays excluded coverage points for a particular CDD file.
 By default, Covered will not display excluded coverage points.  This can be useful when used in
 conjunction with the -x option for including excluded coverage points.  Must be used in
 conjunction with the -d v|d (verbose output) option.
*/
bool report_exclusions = FALSE;

/*!
 If set to a boolean value of TRUE, displays GUI report viewer instead of generating text
 report files.
*/
static bool report_gui = FALSE;

/*!
 If set to a boolean value of TRUE, displays all vector combinational logic on a bitwise
 basis.
*/
bool report_bitwise = FALSE;

/*!
 If set to a boolean value of TRUE, displays combination logic output in a by-line-width
 format (instead of the user specified Verilog source format).
*/
bool flag_use_line_width = FALSE;

/*!
 Specifies the number of characters wide that an expression will allowed to be output for
 if the flag_use_line_width value is set to TRUE.
*/
int line_width = DEFAULT_LINE_WIDTH;

/*!
 If set to a non-zero value, causes Covered to only generate combinational logic
 report information for depths up to the number specified.
*/
unsigned int report_comb_depth = REPORT_SUMMARY;

/*!
 Name of output file to write report contents to.  If output_file is NULL, the report
 will be written to standard output.
*/
char* output_file = NULL;

/*!
 Name of input CDD file to read for generating coverage report.  This value must be
 specified to a value other than NULL for the report phase to complete properly.
*/
static char* input_db = NULL;

/*!
 Suppresses functional units that do not contain any coverage information from being output
 to the report file.
*/
bool flag_suppress_empty_funits = FALSE;

/*!
 Outputs the exclusion ID for an output coverage point.  The exclusion ID can be used by the
 exclude command for excluding/including coverage points.
*/
bool flag_output_exclusion_ids = FALSE;

#ifdef HAVE_TCLTK
/*!
 TCL interpreter for this application.
*/
Tcl_Interp* interp = NULL;
#endif

/*!
 Displays usage information about the report command.
*/
static void report_usage() {

  printf( "\n" );
#ifdef HAVE_TCLTK
  printf( "Usage:  covered report (-h | -view [<database_file>] | [<options>] <database_file>)\n" );
  printf( "\n" );
  printf( "   -view                        Uses the graphical report viewer for viewing reports.  If this\n" );
  printf( "                                 option is not specified, the text report will be generated.\n" );
#else
  printf( "Usage:  covered report (-h | [<options>] <database_file>)\n" );
  printf( "\n" );
#endif
  printf( "   -h                           Displays this help information.\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -m [l][t][c][e][f][r][a][m]  Type(s) of metrics to report.  l=line, t=toggle, c=combinational logic,\n" );
  printf( "                                     e=logic event, f=FSM state/arc, r=race condition, a=assertion, m=memory.  Default is ltcef.\n" );
  printf( "      -d (s|d|v)                   Level of report detail (s=summary, d=detailed, v=verbose).\n" );
  printf( "                                     Default is to display summary coverage information.\n" );
  printf( "      -i                           Provides coverage information for instances instead of module/task/function.\n" );
  printf( "      -c                           If '-d d' or '-d v' is specified, displays covered coverage points.\n" );
  printf( "                                     Default is to display uncovered results.\n" );
  printf( "      -e                           If '-d d' or '-d v' is specified, displays excluded coverage points.\n" );
  printf( "                                     Default is to not display excluded coverage points.\n" );
  printf( "      -o <filename>                File to output report information to.  Default is standard output.\n" );
  printf( "      -w [<line_width>]            Causes expressions to be output to best-fit to the specified line\n" );
  printf( "                                     width.  If the -w option is specified without a value, the default\n" );
  printf( "                                     line width of %d is used.  If the -w option is not specified, all\n", DEFAULT_LINE_WIDTH );
  printf( "                                     expressions are output in the format that the user specified in the\n" );
  printf( "                                     Verilog source.\n" );
  printf( "      -s                           Suppress outputting modules/instances that do not contain any coverage metrics.\n" );
  printf( "      -b                           If combinational logic verbose output is reported and the expression is a\n" );
  printf( "                                     vector operation, this option outputs the coverage information on a bitwise basis.\n" );
  printf( "      -f <filename>                Name of file containing additional arguments to parse.\n" );
  printf( "      -x                           Output exclusion identifiers if the '-d d' or '-d v' options are specified.  The\n" );
  printf( "                                     identifiers can be used with the 'exclude' command for the purposes of\n" );
  printf( "                                     excluding/including coverage points.\n" );
  printf( "\n" );

}

/*!
 Parses the specified string containing the metrics to test.  If
 a legal metric character is found, its corresponding flag is set
 to TRUE.  If a character is found that does not correspond to a
 metric, an error message is flagged to the user (a warning).
*/
static void report_parse_metrics(
  const char* metrics  /*!< Specified metrics to calculate coverage for */
) { PROFILE(REPORT_PARSE_METRICS);

  const char* ptr;  /* Pointer to current character being evaluated */

  /* Set all flags to FALSE */
  report_line        = FALSE;
  report_toggle      = FALSE;
  report_combination = FALSE;
  report_event       = FALSE;
  report_fsm         = FALSE;
  report_race        = FALSE;
  report_assertion   = FALSE;
  report_memory      = FALSE;

  for( ptr=metrics; ptr<(metrics + strlen( metrics )); ptr++ ) {

    switch( *ptr ) {
      case 'l' :
      case 'L' :  report_line        = TRUE;  break;
      case 't' :
      case 'T' :  report_toggle      = TRUE;  break;
      case 'm' :
      case 'M' :  report_memory      = TRUE;  break;
      case 'c' :
      case 'C' :  report_combination = TRUE;  break;
      case 'e' :
      case 'E' :  report_event       = TRUE;  break;
      case 'f' :
      case 'F' :  report_fsm         = TRUE;  break;
      case 'a' :
      case 'A' :  report_assertion   = TRUE;  break;
      case 'r' :
      case 'R' :  report_race        = TRUE;  break;
      default  :
        {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown metric specified '%c'...  Ignoring.", *ptr );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, WARNING, __FILE__, __LINE__ );
        }
        break;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the help option was parsed.

 \throws anonymous Throw Throw Throw Throw Throw Throw Throw Throw

 Parses the argument list for options.  If a legal option is
 found for the report command, the appropriate action is taken for
 that option.  If an option is found that is not allowed, an error
 message is reported to the user and the program terminates immediately.
*/
bool report_parse_args(
  int          argc,      /*!< Number of arguments in argument list argv */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< Argument list passed to this program */
) { PROFILE(REPORT_PARSE_ARGS);

  int  i;  /* Loop iterator */
  bool help_found = FALSE;

  i = last_arg + 1;

  while( (i < argc) && !help_found ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {
 
      report_usage();
      help_found = TRUE;

    } else if( strncmp( "-m", argv[i], 2 ) == 0 ) {
    
      if( check_option_value( argc, argv, i ) ) {
        i++;
        report_parse_metrics( argv[i] );
      } else {
        Throw 0;
      }

    } else if( strncmp( "-view", argv[i], 5 ) == 0 ) {

#ifdef HAVE_TCLTK
      report_gui        = TRUE;
      report_comb_depth = REPORT_VERBOSE;
      report_assertion  = TRUE;
      report_memory     = TRUE;
#else
      print_output( "The -view option is not available with this build", FATAL, __FILE__, __LINE__ );
      Throw 0;
#endif

    } else if( strncmp( "-i", argv[i], 2 ) == 0 ) {

      report_instance = TRUE;

    } else if( strncmp( "-c", argv[i], 2 ) == 0 ) {

      report_covered = TRUE;

    } else if( strncmp( "-e", argv[i], 2 ) == 0 ) {

      report_exclusions = TRUE;

    } else if( strncmp( "-d", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( argv[i][0] == 's' ) {
          report_comb_depth = REPORT_SUMMARY;
        } else if( argv[i][0] == 'd' ) {
          report_comb_depth = REPORT_DETAILED;
        } else if( argv[i][0] == 'v' ) {
          report_comb_depth = REPORT_VERBOSE;
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unrecognized detail type: -d %s", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( output_file != NULL ) {
          print_output( "Only one -o option is allowed on the report command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( is_legal_filename( argv[i] ) ) {
            output_file = strdup_safe( argv[i] );
          } else {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output file \"%s\" is unwritable", argv[i] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-w", argv[i], 2 ) == 0 ) {

      flag_use_line_width = TRUE;

      /* Check to see if user specified a line width value */
      if( ((i+1) < argc) && (sscanf( argv[i+1], "%d", &line_width ) == 1) ) {
        i++;
      } else {
        line_width = DEFAULT_LINE_WIDTH;
      }

    } else if( strncmp( "-s", argv[i], 2 ) == 0 ) {

      flag_suppress_empty_funits = TRUE;

    } else if( strncmp( "-b", argv[i], 2 ) == 0 ) {

      report_bitwise = TRUE;

    } else if( strncmp( "-f", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        char**       arg_list = NULL;
        int          arg_num  = 0;
        unsigned int j;
        i++;
        Try {
          read_command_file( argv[i], &arg_list, &arg_num );
          help_found = report_parse_args( arg_num, -1, (const char**)arg_list );
        } Catch_anonymous {
          for( j=0; j<arg_num; j++ ) {
            free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
          }
          free_safe( arg_list, (sizeof( char* ) * arg_num) );
          Throw 0;
        }
        for( j=0; j<arg_num; j++ ) {
          free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
        }
        free_safe( arg_list, (sizeof( char* ) * arg_num) );
      } else {
        Throw 0;
      }

    } else if( strncmp( "-x", argv[i], 2 ) == 0 ) {

      flag_output_exclusion_ids = TRUE;
 
    } else if( (i + 1) == argc ) {

      if( file_exists( argv[i] ) ) {
     
        input_db = strdup_safe( argv[i] );
 
      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Cannot find %s database file for opening", argv[i] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    } else {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown report command option \"%s\".  See \"covered -h\" for more information.", argv[i] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    }

    i++;

  }

  if( !help_found ) {

    /*
     If the user has specified the -x and -c (but not -e), we are not going to be outputting exclusion IDs
     because covered items cannot be excluded.  So... we will set flag_output_exclusion_ids to false and
     output a warning message.
    */
    if( flag_output_exclusion_ids && report_covered && !report_exclusions ) {
      flag_output_exclusion_ids = FALSE;
      print_output( "The -x and -c options were specified.  Covered items cannot be excluded so no exclusion IDs will be output.", WARNING, __FILE__, __LINE__ );
    }

  }

  PROFILE_END;

  return( help_found );

}

/*!
 Recursively parses instance tree, creating statistic structures for each
 of the instances in the tree.  Calculates summary coverage information for
 children nodes first and parent nodes after.  In this way, the parent nodes
 will have the accumulated information from themselves and all of their
 children.
*/
void report_gather_instance_stats(
  funit_inst* root  /*!< Pointer to root of instance tree to search */
) { PROFILE(REPORT_GATHER_INSTANCE_STATS);

  funit_inst* curr;        /* Pointer to current instance being evaluated */

  /* Create and initialize statistic structure */
  statistic_create( &(root->stat) );

  /* Get coverage results for all children first */
  curr = root->child_head;
  while( curr != NULL ) {
    report_gather_instance_stats( curr );
    curr = curr->next;
  }

  /* If this module is an OVL module or it isn't attached to a functional unit, don't get coverage statistics */
  if( (root->funit != NULL) && ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    /* Get coverage results for this instance */
    if( report_line && (info_suppl.part.scored_line == 1) ) {
      line_get_stats( root->funit,
                      &(root->stat->line_hit),
                      &(root->stat->line_excluded),
                      &(root->stat->line_total) );
    }

    if( report_toggle && (info_suppl.part.scored_toggle == 1) ) {
      toggle_get_stats( root->funit,
                        &(root->stat->tog01_hit), 
                        &(root->stat->tog10_hit),
                        &(root->stat->tog_excluded),
                        &(root->stat->tog_total), 
                        &(root->stat->tog_cov_found) );
    }

    if( (report_combination || report_event) && ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_events == 1)) ) {
      combination_get_stats( root->funit,
                             report_combination,
                             report_event,
                             &(root->stat->comb_hit),
                             &(root->stat->comb_excluded),
                             &(root->stat->comb_total) );
    }

    if( report_fsm && (info_suppl.part.scored_fsm == 1) ) {
      fsm_get_stats( root->funit->fsms,
                     root->funit->fsm_size,
                     &(root->stat->state_hit),
                     &(root->stat->state_total),
                     &(root->stat->arc_hit),
                     &(root->stat->arc_total),
                     &(root->stat->arc_excluded) );
    }

    if( report_assertion && (info_suppl.part.scored_assert == 1) ) {
      assertion_get_stats( root->funit,
                           &(root->stat->assert_hit),
                           &(root->stat->assert_excluded),
                           &(root->stat->assert_total) );
    }

    if( report_memory && (info_suppl.part.scored_memory == 1) ) {
      memory_get_stats( root->funit,
                        &(root->stat->mem_wr_hit),
                        &(root->stat->mem_rd_hit),
                        &(root->stat->mem_ae_total),
                        &(root->stat->mem_tog01_hit),
                        &(root->stat->mem_tog10_hit),
                        &(root->stat->mem_tog_total),
                        &(root->stat->mem_excluded),
                        &(root->stat->mem_cov_found) );
    }

    /* Only get race condition statistics for this instance module if the module hasn't been gathered yet */
    if( report_race && (root->funit->stat == NULL) ) {
      statistic_create( &(root->funit->stat) );
      race_get_stats( root->funit->race_head,
                      &(root->funit->stat->race_total),
                      &(root->funit->stat->rtype_total) );
    }

  }

  /* Set show bit */
  if( flag_suppress_empty_funits ) {
    root->stat->show = !statistic_is_empty( root->stat );
  }

  PROFILE_END;

}

/*!
 Traverses functional unit list, creating statistic structures for each
 of the functional units in the tree, and calculates summary coverage information.
*/
static void report_gather_funit_stats(
  funit_link* head  /*!< Pointer to head of functional unit list to search */
) { PROFILE(REPORT_GATHER_FUNIT_STATS);

  while( head != NULL ) {

    statistic_create( &(head->funit->stat) );

    /* If this module is an OVL assertion module, don't get statistics for it */
    if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit ) ) {

      /* Get coverage results for this instance */
      if( report_line && (info_suppl.part.scored_line == 1) ) {
        line_get_stats( head->funit,
                        &(head->funit->stat->line_hit),
                        &(head->funit->stat->line_excluded),
                        &(head->funit->stat->line_total) );
      }

      if( report_toggle && (info_suppl.part.scored_toggle == 1) ) {
        toggle_get_stats( head->funit,
                          &(head->funit->stat->tog01_hit), 
                          &(head->funit->stat->tog10_hit),
                          &(head->funit->stat->tog_excluded),
                          &(head->funit->stat->tog_total), 
                          &(head->funit->stat->tog_cov_found) );
      }

      if( (report_combination || report_event) && ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_events == 1)) ) {
        combination_get_stats( head->funit,
                               report_combination,
                               report_event,
                               &(head->funit->stat->comb_hit),
                               &(head->funit->stat->comb_excluded),
                               &(head->funit->stat->comb_total) );
      }

      if( report_fsm && (info_suppl.part.scored_fsm == 1) ) {
        fsm_get_stats( head->funit->fsms,
                       head->funit->fsm_size,
                       &(head->funit->stat->state_hit),
                       &(head->funit->stat->state_total),
                       &(head->funit->stat->arc_hit) ,
                       &(head->funit->stat->arc_total),
                       &(head->funit->stat->arc_excluded) );
      }

      if( report_assertion && (info_suppl.part.scored_assert == 1) ) {
        assertion_get_stats( head->funit,
                             &(head->funit->stat->assert_hit),
                             &(head->funit->stat->assert_excluded),
                             &(head->funit->stat->assert_total) );
      }

      if( report_memory && (info_suppl.part.scored_memory == 1) ) {
        memory_get_stats( head->funit,
                          &(head->funit->stat->mem_wr_hit),
                          &(head->funit->stat->mem_rd_hit),
                          &(head->funit->stat->mem_ae_total),
                          &(head->funit->stat->mem_tog01_hit),
                          &(head->funit->stat->mem_tog10_hit),
                          &(head->funit->stat->mem_tog_total),
                          &(head->funit->stat->mem_excluded),
                          &(head->funit->stat->mem_cov_found) );
      }

      if( report_race ) {
        race_get_stats( head->funit->race_head,
                        &(head->funit->stat->race_total),
                        &(head->funit->stat->rtype_total) );
      }

    }

    /* Set show bit */
    if( flag_suppress_empty_funits ) {
      head->funit->stat->show = !statistic_is_empty( head->funit->stat );
    }

    head = head->next;

  }

  PROFILE_END;

}

/*!
 Generates generic report header for all reports.
*/
void report_print_header(
  FILE* ofile  /*!< Pointer to output stream to display report information to */
) { PROFILE(REPORT_PRINT_HEADER);

  int i;  /* Loop iterator */

  switch( report_comb_depth ) {
    case REPORT_SUMMARY  :
      fprintf( ofile, "                            :::::::::::::::::::::::::::::::::::::::::::::::::::::\n" );
      fprintf( ofile, "                            ::                                                 ::\n" );
      fprintf( ofile, "                            ::  Covered -- Verilog Coverage Summarized Report  ::\n" );
      fprintf( ofile, "                            ::                                                 ::\n" );
      fprintf( ofile, "                            :::::::::::::::::::::::::::::::::::::::::::::::::::::\n\n\n" );
      break;
    case REPORT_DETAILED :
      fprintf( ofile, "                             :::::::::::::::::::::::::::::::::::::::::::::::::::\n" );
      fprintf( ofile, "                             ::                                               ::\n" );
      fprintf( ofile, "                             ::  Covered -- Verilog Coverage Detailed Report  ::\n" );
      fprintf( ofile, "                             ::                                               ::\n" );
      fprintf( ofile, "                             :::::::::::::::::::::::::::::::::::::::::::::::::::\n\n\n" );
      break;
    case REPORT_VERBOSE  :
      fprintf( ofile, "                             ::::::::::::::::::::::::::::::::::::::::::::::::::\n" );
      fprintf( ofile, "                             ::                                              ::\n" );
      fprintf( ofile, "                             ::  Covered -- Verilog Coverage Verbose Report  ::\n" );
      fprintf( ofile, "                             ::                                              ::\n" );
      fprintf( ofile, "                             ::::::::::::::::::::::::::::::::::::::::::::::::::\n\n\n" );
      break;
    default              :  break;
  }

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   GENERAL INFORMATION   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  fprintf( ofile, "* Report generated from CDD file : %s\n\n", input_db );

  /* If the CDD contains a user-supplied message output it here, formatting it as needed. */
  if( cdd_message != NULL ) {
    fprintf( ofile, "* User-supplied CDD message      :\n" );
    report_output_exclusion_reason( ofile, 4, cdd_message, FALSE );
  }

  if( report_instance ) {
    fprintf( ofile, "* Reported by                    : Instance\n\n" );
  } else {
    fprintf( ofile, "* Reported by                    : Module\n\n" );
  }

  if( flag_output_exclusion_ids ) {
    fprintf( ofile, "* Report contains exclusion IDs (value within parenthesis preceding verbose output)\n\n" );
  }

  if( (info_suppl.part.excl_assign == 1) ||
      (info_suppl.part.excl_always == 1) ||
      (info_suppl.part.excl_init   == 1) ||
      (info_suppl.part.excl_final  == 1) ||
      (info_suppl.part.excl_pragma == 1) ) {
    fprintf( ofile, "* CDD file excludes the following block types:\n" );
    if( info_suppl.part.excl_assign == 1 ) {
      fprintf( ofile, "    assign - Continuous Assignments\n" );
    }
    if( info_suppl.part.excl_always == 1 ) {
      fprintf( ofile, "    always - Always Statements\n" );
    }
    if( info_suppl.part.excl_init == 1 ) {
      fprintf( ofile, "    initial - Initial Statements\n" );
    }
    if( info_suppl.part.excl_final == 1 ) {
      fprintf( ofile, "    final - Final Statements\n" );
    }
    if( info_suppl.part.excl_pragma == 1 ) {
      fprintf( ofile, "    pragma - Code surrounded by coverage off/on pragmas\n" );
    }
    fprintf( ofile, "\n" );
  }

  if( merge_in_head != NULL ) {

    if( merge_in_head == merge_in_tail ) {

      char* file;

      fprintf( ofile, "* Report generated from CDD file that was merged from the following files with the following leading hierarchies:\n" );
      fprintf( ofile, "    Filename                                           Leading Hierarchy\n" );
      fprintf( ofile, "    -----------------------------------------------------------------------------------------------------------------\n" );
      fprintf( ofile, "    %-49.49s  %-62.62s\n", input_db,           db_list[curr_db]->leading_hierarchies[0] );
      file = get_relative_path( merge_in_head->str );
      fprintf( ofile, "    %-49.49s  %-62.62s\n", file, db_list[curr_db]->leading_hierarchies[1] ); 
      free_safe( file, (strlen( file ) + 1) );

      if( report_instance && db_list[curr_db]->leading_hiers_differ ) {
        fprintf( ofile, "\n* Merged CDD files contain different leading hierarchies, will use value \"<NA>\" to represent leading hierarchy.\n\n" );
      }

    } else {

      str_link* strl = merge_in_head;

      fprintf( ofile, "* Report generated from CDD file that was merged from the following files:\n" );
      fprintf( ofile, "    Filename                                           Leading Hierarchy\n" );
      fprintf( ofile, "    -----------------------------------------------------------------------------------------------------------------\n" );

      i = 1;
      while( strl != NULL ) {
        char* file = get_relative_path( strl->str );
        fprintf( ofile, "    %-49.49s  %-62.62s\n", file, db_list[curr_db]->leading_hierarchies[i++] );
        free_safe( file, (strlen( file ) + 1) );
        strl = strl->next;
      }

      if( report_instance && db_list[curr_db]->leading_hiers_differ ) {
        fprintf( ofile, "\n* Merged CDD files contain different leading hierarchies, will use value \"<NA>\" to represent leading hierarchy.\n\n" );
      }

    }

    fprintf( ofile, "\n" );

  }

  PROFILE_END;

}

/*!
 \throws anonymous combination_report

 Generates a coverage report based on the options specified on the command line
 to the specified output stream.
*/
static void report_generate(
  FILE* ofile  /*!< Pointer to output stream to display report information to */
) { PROFILE(REPORT_GENERATE);

  report_print_header( ofile );

  /* Gather statistics first */
  if( report_instance ) {
    inst_link* instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      report_gather_instance_stats( instl->inst );
      instl = instl->next;
    }
  } else {
    report_gather_funit_stats( db_list[curr_db]->funit_head );
  }

  /* Call out the proper reports for the specified metrics to report */
  if( report_line ) {
    if( info_suppl.part.scored_line ) {
      line_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
    } else {
      print_output( "Line reporting requested when line coverage was not accumulated during scoring", WARNING, __FILE__, __LINE__ );
    }
  }

  if( report_toggle ) {
    if( info_suppl.part.scored_toggle ) {
      toggle_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
    } else {
      print_output( "Toggle reporting requested when toggle coverage was not accumulated during scoring", WARNING, __FILE__, __LINE__ );
    }
  }

  if( report_memory ) {
    if( info_suppl.part.scored_memory ) {
      memory_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
    } else {
      print_output( "Memory reporting requested when memory coverage was not accumulated during scoring", WARNING, __FILE__, __LINE__ );
    }
  }

  if( report_combination || report_event ) {
    if( info_suppl.part.scored_comb || info_suppl.part.scored_events ) {
      combination_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
    } else {
      print_output( "Combinational logic reporting requested when combinational logic coverage was not accumulated during scoring", WARNING, __FILE__, __LINE__ );
    }
  }

  if( report_fsm ) {
    if( info_suppl.part.scored_fsm ) {
      fsm_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
    } else {
      print_output( "FSM reporting requested when FSM coverage was not accumulated during scoring", WARNING, __FILE__, __LINE__ );
    }
  }

  if( report_assertion ) {
    if( info_suppl.part.scored_assert ) {
      assertion_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
    } else {
      print_output( "Assertion reporting requested when assertion coverage was not accumulated during scoring", WARNING, __FILE__, __LINE__ );
    }
  }

  if( report_race ) {
    race_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if CDD file was read properly; otherwise, returns FALSE.

 \throws anonymous db_read Throw bind_perform

 Reads in specified CDD file and gathers functional unit statistics to get ready for GUI
 interaction with this CDD file. 
*/
void report_read_cdd_and_ready(
  const char* ifile  /*!< Name of CDD file to read from */
) { PROFILE(REPORT_READ_CDD_AND_READY);

  /* Open database file for reading */
  if( (ifile == NULL) || (ifile[0] == '\0') ) {

    print_output( "CDD file name was not specified for reading", FATAL, __FILE__, __LINE__ );
    Throw 0;

  } else {

    inst_link* instl;
    bool       first = (db_size == 0);

    /* Read in database, performing instance merging */
    curr_db = 0;
    (void)db_read( ifile, (first ? READ_MODE_REPORT_NO_MERGE : READ_MODE_MERGE_INST_MERGE) );
    bind_perform( TRUE, 0 );

    /* Gather instance statistics */
    instl = db_list[0]->inst_head;
    while( instl != NULL ) {
      report_gather_instance_stats( instl->inst );
      instl = instl->next;
    }

    /* Read in database again, performing module merging */
    curr_db = 1;
    (void)db_read( ifile, READ_MODE_REPORT_MOD_MERGE );
    bind_perform( TRUE, 0 );

    /* Now merge functional units and gather module statistics */
    report_gather_funit_stats( db_list[1]->funit_head );

    /* Set the current database back to 0 */
    curr_db = 0;

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if CDD file was closed properly; otherwise, returns FALSE.

 Closes the currently loaded CDD file.
*/
void report_close_cdd() { PROFILE(REPORT_CLOSE_CDD);

  db_close();

  PROFILE_END;

}

/*!
 \throws anonymous db_write

 Saves the currently loaded CDD database to the given filename.
*/
void report_save_cdd(
  const char* filename  /*!< Name to use for saving the currently loaded filename */
) { PROFILE(REPORT_SAVE_CDD);

  /* Write the instance database */
  curr_db = 1;

  db_write( filename, FALSE, FALSE );

  /* Restore the database */
  curr_db = 0;

  PROFILE_END;

}

#ifdef OBSOLETE
/*!
 \return Returns a formatted exclude reason ready to be output.
*/
char* report_format_exclusion_reason(
  int   leading_spaces,  /*!< Number of leading spaces (for formatting purposes) */
  char* msg,             /*!< Message to display (no newlines and only one space between each word) */
  bool  header,          /*!< If set to TRUE, display a header before outputting; otherwise, avoid the header */
  int   width            /*!< Width (in characters) to restrict the output to */
) { PROFILE(REPORT_FORMAT_EXCLUSION_REASON);

  char* msg_cpy;
  char* msg_tcpy;
  char* lead_sp;
  int   curr_width;
  char* word;

  /* Copy the message */
  msg_cpy  = strdup_safe( msg );
  msg_tcpy = msg_cpy;

  /* Allocate and populate the leading spaces string */
  lead_sp = (char*)malloc_safe( leading_spaces + 1 );
  gen_char_string( lead_sp, ' ', leading_spaces );

  /* Output message */
  if( header ) {
    fprintf( ofile, "\n%sReason:  ", lead_sp );
  } else {
    fprintf( ofile, "\n%s", lead_sp );
  }

  curr_width = leading_spaces + 9;
  word       = msg_cpy;
  while( *msg_cpy != '\0' ) {
    /* Get the next token */
    while( (*msg_cpy != '\0') && (*msg_cpy != ' ') ) msg_cpy++;
    if( *msg_cpy == ' ' ) {
      *msg_cpy = '\0';
      msg_cpy++;
    }
    if( (strlen( word ) + curr_width) > line_width ) {
      if( header ) {
        fprintf( ofile, "\n%s         ", lead_sp );
      } else {
        fprintf( ofile, "\n%s", lead_sp );
      }
      curr_width = leading_spaces + 9;
    }
    fprintf( ofile, "%s ", word );
    if( word[strlen(word)-1] == '.' ) {
      fprintf( ofile, " " );
    }
    curr_width += strlen( word ) + 1;
    word = msg_cpy;
  }
  fprintf( ofile, "\n\n" );

  /* Deallocate memory */
  free_safe( msg_tcpy, (strlen( msg ) + 1) );
  free_safe( lead_sp, (strlen( lead_sp ) + 1) );

  PROFILE_END;

  return( msg );

}
#endif

/*!
 Outputs the given exclude report message to the specified output stream, handling the appropriate formatting.
*/
void report_output_exclusion_reason(
  FILE* ofile,           /*!< Output file stream */
  int   leading_spaces,  /*!< Number of leading spaces (for formatting purposes) */
  char* msg,             /*!< Message to display (no newlines and only one space between each word) */
  bool  header           /*!< If set to TRUE, display a header before outputting; otherwise, avoid the header */
) { PROFILE(REPORT_OUTPUT_EXCLUSION_REASON);

  char* msg_cpy;
  char* msg_tcpy;
  char* lead_sp;
  int   curr_width;
  char* word;

  /* Copy the message */
  msg_cpy  = strdup_safe( msg );
  msg_tcpy = msg_cpy;

  /* Allocate and populate the leading spaces string */
  lead_sp = (char*)malloc_safe( leading_spaces + 1 );
  gen_char_string( lead_sp, ' ', leading_spaces );

  /* Output message */
  if( header ) {
    fprintf( ofile, "\n%sReason:  ", lead_sp );
  } else {
    fprintf( ofile, "\n%s", lead_sp );
  }

  curr_width = leading_spaces + 9;
  word       = msg_cpy;
  while( *msg_cpy != '\0' ) {
    /* Get the next token */
    while( (*msg_cpy != '\0') && (*msg_cpy != ' ') ) msg_cpy++;
    if( *msg_cpy == ' ' ) {
      *msg_cpy = '\0';
      msg_cpy++;
    }
    if( (strlen( word ) + curr_width) > line_width ) {
      if( header ) {
        fprintf( ofile, "\n%s         ", lead_sp );
      } else {
        fprintf( ofile, "\n%s", lead_sp );
      }
      curr_width = leading_spaces + 9;
    }
    fprintf( ofile, "%s ", word );
    if( word[strlen(word)-1] == '.' ) {
      fprintf( ofile, " " );
    }
    curr_width += strlen( word ) + 1;
    word = msg_cpy;
  }
  fprintf( ofile, "\n\n" );

  /* Deallocate memory */
  free_safe( msg_tcpy, (strlen( msg ) + 1) );
  free_safe( lead_sp, (strlen( lead_sp ) + 1) );

  PROFILE_END;

}

/*!
 Performs report command functionality.
*/
void command_report(
  int          argc,      /*!< Number of arguments in report command-line */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< Arguments passed to report command to parse */
) { PROFILE(COMMAND_REPORT);

  FILE*        ofile           = NULL;  /* Pointer to output stream */
#ifdef HAVE_TCLTK
  char*        covered_home    = NULL;  /* Pathname to Covered's home installation directory */
  char*        covered_browser;         /* Name of browser to use for GUI help pages */
  char*        covered_version;         /* String version of current Covered version */
  char*        main_file       = NULL;  /* Name of main TCL file to interpret */ 
  char*        user_home;               /* HOME environment variable */
#endif
  unsigned int rv;                      /* Return value from snprintf calls */
  bool         error           = FALSE;

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, HEADER, __FILE__, __LINE__ );

  Try {

    /* Parse score command-line */
    if( !report_parse_args( argc, last_arg, argv ) ) {

      if( !report_gui ) {

        /* Open database file for reading */
        if( input_db == NULL ) {

          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Database file not specified in command line" );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;

        } else {

          /* Read in CDD file */
          (void)db_read( input_db, (report_instance ? READ_MODE_REPORT_NO_MERGE : READ_MODE_REPORT_MOD_MERGE) );

          /* Perform binding */
          bind_perform( TRUE, 0 );

          Try {

            /* Open output stream */
            if( output_file != NULL ) {
              ofile = fopen( output_file, "w" );
              assert( ofile != NULL );  /* We should have already verified that the file is writable */
            } else {
              ofile = stdout;
            }

            /* Generate report */
            report_generate( ofile );

          } Catch_anonymous {
            if( ofile != stdout ) {
              rv = fclose( ofile );
              assert( rv == 0 );
            }
            Throw 0;
          }

          /* Close output file */
          if( ofile != stdout ) {
            rv = fclose( ofile );
            assert( rv == 0 );
          }

        }

#ifdef HAVE_TCLTK
      } else {

        unsigned int slen;

        Try {

          unsigned int rv;

          /* Initialize the Tcl/Tk interpreter */
          interp = Tcl_CreateInterp();
          assert( interp );

          if( Tcl_Init( interp ) == TCL_ERROR ) {
            printf( "ERROR: %s\n", interp->result );
            Throw 0;
          }

          if( Tk_SafeInit( interp ) == TCL_ERROR ) {
            printf( "ERROR: %s\n", interp->result );
            Throw 0;
          }

          /* Get the COVERED_HOME environment variable */
#ifndef INSTALL_DIR
          if( getenv( "COVERED_HOME" ) == NULL ) {
            print_output( "COVERED_HOME not initialized.  Exiting...", FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
          covered_home = strdup_safe( getenv( "COVERED_HOME" ) );
#else
          covered_home = strdup_safe( INSTALL_DIR );
#endif

          /* Get the COVERED_BROWSER environment variable */
#ifndef COVERED_BROWSER
          covered_browser = strdup_safe( getenv( "COVERED_BROWSER" ) );
#else
          covered_browser = strdup_safe( COVERED_BROWSER );
#endif

          covered_version = strdup_safe( VERSION );
          user_home       = getenv( "HOME" );

          /* Initialize TCL */
          tcl_func_initialize( interp, argv[0], user_home, covered_home, covered_version, covered_browser, input_db );

          /* Call the top-level Tcl file */
          slen      = strlen( covered_home ) + 30;
          main_file = (char*)malloc_safe( slen );
          rv        = snprintf( main_file, slen, "%s/scripts/main_view.tcl", covered_home );
          assert( rv < slen );
          rv = Tcl_EvalFile( interp, main_file );
          if( rv != TCL_OK ) {
            rv = snprintf( user_msg, USER_MSG_LENGTH, "TCL/TK: %s\n", Tcl_ErrnoMsg( Tcl_GetErrno() ) );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }

          /* Call the main-loop */
          Tk_MainLoop ();

        } Catch_anonymous {
          free_safe( covered_home,    (strlen( covered_home ) + 1) );
          free_safe( main_file,       slen );
          free_safe( covered_browser, (strlen( covered_browser ) + 1) );
          free_safe( covered_version, (strlen( covered_version ) + 1) );
          Throw 0;
        }

        /* Clean Up */
        free_safe( covered_home,    (strlen( covered_home ) + 1) );
        free_safe( main_file,       slen );
        free_safe( covered_browser, (strlen( covered_browser ) + 1) );
        free_safe( covered_version, (strlen( covered_version ) + 1) );
#endif

      }

    }

  } Catch_anonymous {
    error = TRUE;
  }

  free_safe( output_file, (strlen( output_file ) + 1) );
  free_safe( input_db, (strlen( input_db ) + 1) );

  /* Close the database */
  db_close();

  if( error ) {
    Throw 0;
  }

  PROFILE_END;

}

