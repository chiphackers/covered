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


extern char        user_msg[USER_MSG_LENGTH];
extern inst_link*  inst_head;
extern funit_link* funit_head;
extern int         merge_in_num;
extern char**      merge_in;
extern char**      leading_hierarchies;
extern int         leading_hier_num;
extern bool        leading_hiers_differ;
extern isuppl      info_suppl;

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
 the -v (verbose output) option.
*/
bool report_covered = FALSE;

/*!
 If set to a boolean value of TRUE, displays GUI report viewer instead of generating text
 report files.
*/
bool report_gui = FALSE;

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

#ifdef HAVE_TCLTK
/*!
 TCL interpreter for this application.
*/
Tcl_Interp* interp;
#endif

/*!
 Displays usage information about the report command.
*/
static void report_usage() {

  printf( "\n" );
#ifdef HAVE_TCLTK
  printf( "Usage:  covered report (-h | -view | [<options>] <database_file>)\n" );
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
  printf( "      -m [l][t][c][f][r][a][m]  Type(s) of metrics to report.  l=line, t=toggle, c=combinational logic,\n" );
  printf( "                                 f=FSM state/arc, r=race condition, a=assertion, m=memory.  Default is ltcf.\n" );
  printf( "      -d (s|d|v)                Level of report detail (s=summary, d=detailed, v=verbose).\n" );
  printf( "                                 Default is to display summary coverage information.\n" );
  printf( "      -i                        Provides coverage information for instances instead of module/task/function.\n" );
  printf( "      -c                        If '-d d' or '-d v' is specified, displays covered line, toggle\n" );
  printf( "                                 and combinational cases.  Default is to display uncovered results.\n" );
  printf( "      -o <filename>             File to output report information to.  Default is standard output.\n" );
  printf( "      -w [<line_width>]         Causes expressions to be output to best-fit to the specified line\n" );
  printf( "                                 width.  If the -w option is specified without a value, the default\n" );
  printf( "                                 line width of 80 is used.  If the -w option is not specified, all\n" );
  printf( "                                 expressions are output in the format that the user specified in the\n" );
  printf( "                                 Verilog source.\n" );
  printf( "      -s                        Suppress outputting modules/instances that do not contain any coverage metrics.\n" );
  printf( "      -b                        If combinational logic verbose output is reported and the expression is a\n" );
  printf( "                                 vector operation, this option outputs the coverage information on a bitwise basis.\n" );
  printf( "\n" );

}

/*!
 \param metrics  Specified metrics to calculate coverage for.

 Parses the specified string containing the metrics to test.  If
 a legal metric character is found, its corresponding flag is set
 to TRUE.  If a character is found that does not correspond to a
 metric, an error message is flagged to the user (a warning).
*/
static void report_parse_metrics( const char* metrics ) { PROFILE(REPORT_PARSE_METRICS);

  const char* ptr;  /* Pointer to current character being evaluated */

  /* Set all flags to FALSE */
  report_line        = FALSE;
  report_toggle      = FALSE;
  report_combination = FALSE;
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
      case 'c' :
      case 'C' :  report_combination = TRUE;  break;
      case 'f' :
      case 'F' :  report_fsm         = TRUE;  break;
      case 'r' :
      case 'R' :  report_race        = TRUE;  break;
      case 'a' :
      case 'A' :  report_assertion   = TRUE;  break;
      case 'm' :
      case 'M' :  report_memory      = TRUE;  break;
      default  :
        {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown metric specified '%c'...  Ignoring.", *ptr );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, WARNING, __FILE__, __LINE__ );
        }
        break;
    }

  }

}

/*!
 \param argc      Number of arguments in argument list argv.
 \param last_arg  Index of last parsed argument from list.
 \param argv      Argument list passed to this program.

 \return Returns TRUE if parsing was successful; otherwise, returns FALSE.

 Parses the argument list for options.  If a legal option is
 found for the report command, the appropriate action is taken for
 that option.  If an option is found that is not allowed, an error
 message is reported to the user and the program terminates immediately.
*/
bool report_parse_args( int argc, int last_arg, const char** argv ) { PROFILE(REPORT_PARSE_ARGS);

  bool retval = TRUE;  /* Return value for this function */
  int  i;              /* Loop iterator */
  int  chars_read;     /* Number of characters read in from sscanf */

  i = last_arg + 1;

  while( (i < argc) && retval ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {
 
      report_usage();
      retval = FALSE;

    } else if( strncmp( "-m", argv[i], 2 ) == 0 ) {
    
      if( (retval = check_option_value( argc, argv, i )) ) {
        i++;
        report_parse_metrics( argv[i] );
      }

    } else if( strncmp( "-view", argv[i], 5 ) == 0 ) {

#ifdef HAVE_TCLTK
      report_gui          = TRUE;
      report_comb_depth   = REPORT_VERBOSE;
      report_assertion    = TRUE;
      report_memory       = TRUE;
//      flag_use_line_width = TRUE;
#else
      print_output( "The -view option is not available with this build", FATAL, __FILE__, __LINE__ );
      retval = FALSE;
#endif

    } else if( strncmp( "-i", argv[i], 2 ) == 0 ) {

      report_instance = TRUE;

    } else if( strncmp( "-c", argv[i], 2 ) == 0 ) {

      report_covered = TRUE;

    } else if( strncmp( "-d", argv[i], 2 ) == 0 ) {

      if( (retval = check_option_value( argc, argv, i )) ) {
        i++;
        if( argv[i][0] == 's' ) {
          report_comb_depth = REPORT_SUMMARY;
        } else if( argv[i][0] == 'd' ) {
          report_comb_depth = REPORT_DETAILED;
        } else if( argv[i][0] == 'v' ) {
          report_comb_depth = REPORT_VERBOSE;
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unrecognized detail type: -d %s\n", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          retval = FALSE;
        }
      }

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      if( (retval = check_option_value( argc, argv, i )) ) {
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
            retval = FALSE;
          }
        }
      }

    } else if( strncmp( "-w", argv[i], 2 ) == 0 ) {

      flag_use_line_width = TRUE;

      /* Check to see if user specified a line width value */
      if( ((i+1) < argc) && (sscanf( argv[i+1], "%d%n", &line_width, &chars_read ) == 1) ) {
        if( strlen( argv[i+1] ) != chars_read ) {
          line_width = DEFAULT_LINE_WIDTH;
        } else {
          i++;
        }
      } else {
        line_width = DEFAULT_LINE_WIDTH;
      }

    } else if( strncmp( "-s", argv[i], 2 ) == 0 ) {

      flag_suppress_empty_funits = TRUE;

    } else if( strncmp( "-b", argv[i], 2 ) == 0 ) {

      report_bitwise = TRUE;

    } else if( (i + 1) == argc ) {

      if( file_exists( argv[i] ) ) {
     
        input_db = strdup_safe( argv[i] );
 
      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Cannot find %s database file for opening", argv[i] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( EXIT_FAILURE );

      }

    } else {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown report command option \"%s\".  See \"covered -h\" for more information.", argv[i] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = FALSE;

    }

    i++;

  }

  return( retval );

}

/*!
 \param root   Pointer to root of instance tree to search.
 
 Recursively parses instance tree, creating statistic structures for each
 of the instances in the tree.  Calculates summary coverage information for
 children nodes first and parent nodes after.  In this way, the parent nodes
 will have the accumulated information from themselves and all of their
 children.
*/
static void report_gather_instance_stats( funit_inst* root ) { PROFILE(REPORT_GATHER_INSTANCE_STATS);

  funit_inst* curr;        /* Pointer to current instance being evaluated */

  /* Create a statistics structure for this instance */
  assert( root->stat == NULL );

  root->stat = statistic_create();

  /* Get coverage results for all children first */
  curr = root->child_head;
  while( curr != NULL ) {
    report_gather_instance_stats( curr );
    curr = curr->next;
  }

  /* If this module is an OVL module, don't get coverage statistics */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    /* Get coverage results for this instance */
    if( report_line ) {
      line_get_stats( root->funit, &(root->stat->line_total), &(root->stat->line_hit) );
    }

    if( report_toggle ) {
      toggle_get_stats( root->funit->sig_head, 
                        &(root->stat->tog_total), 
                        &(root->stat->tog01_hit), 
                        &(root->stat->tog10_hit) );
    }

    if( report_combination ) {
      combination_get_stats( root->funit,
                             &(root->stat->comb_total),
                             &(root->stat->comb_hit) );
    }

    if( report_fsm ) {
      fsm_get_stats( root->funit->fsm_head,
                     &(root->stat->state_total),
                     &(root->stat->state_hit),
                     &(root->stat->arc_total),
                     &(root->stat->arc_hit) );
    }

    if( report_assertion ) {
      assertion_get_stats( root->funit,
                           &(root->stat->assert_total),
                           &(root->stat->assert_hit) );
    }

    if( report_memory ) {
      memory_get_stats( root->funit->sig_head,
                        &(root->stat->mem_ae_total),
                        &(root->stat->mem_wr_hit),
                        &(root->stat->mem_rd_hit),
                        &(root->stat->mem_tog_total),
                        &(root->stat->mem_tog01_hit),
                        &(root->stat->mem_tog10_hit) );
    }

    /* Only get race condition statistics for this instance module if the module hasn't been gathered yet */
    if( report_race && (root->funit->stat == NULL) ) {
      root->funit->stat = statistic_create();
      race_get_stats( root->funit->race_head,
                      &(root->funit->stat->race_total),
                      &(root->funit->stat->rtype_total) );
    }

  }

  /* Set show bit */
  if( flag_suppress_empty_funits ) {
    root->stat->show = !statistic_is_empty( root->stat );
  }

}

/*!
 \param head  Pointer to head of functional unit list to search.
 
 Traverses functional unit list, creating statistic structures for each
 of the functional units in the tree, and calculates summary coverage information.
*/
static void report_gather_funit_stats( funit_link* head ) { PROFILE(REPORT_GATHER_FUNIT_STATS);

  while( head != NULL ) {

    head->funit->stat = statistic_create();

    /* If this module is an OVL assertion module, don't get statistics for it */
    if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit ) ) {

      /* Get coverage results for this instance */
      if( report_line ) {
        line_get_stats( head->funit, &(head->funit->stat->line_total), &(head->funit->stat->line_hit) );
      }

      if( report_toggle ) {
        toggle_get_stats( head->funit->sig_head, 
                          &(head->funit->stat->tog_total), 
                          &(head->funit->stat->tog01_hit), 
                          &(head->funit->stat->tog10_hit) );
      }

      if( report_combination ) {
        combination_get_stats( head->funit,
                               &(head->funit->stat->comb_total),
                               &(head->funit->stat->comb_hit) );
      }

      if( report_fsm ) {
        fsm_get_stats( head->funit->fsm_head,
                       &(head->funit->stat->state_total),
                       &(head->funit->stat->state_hit),
                       &(head->funit->stat->arc_total),
                       &(head->funit->stat->arc_hit) );
      }

      if( report_assertion ) {
        assertion_get_stats( head->funit,
                             &(head->funit->stat->assert_total),
                             &(head->funit->stat->assert_hit) );
      }

      if( report_memory ) {
        memory_get_stats( head->funit->sig_head,
                          &(head->funit->stat->mem_ae_total),
                          &(head->funit->stat->mem_wr_hit),
                          &(head->funit->stat->mem_rd_hit),
                          &(head->funit->stat->mem_tog_total),
                          &(head->funit->stat->mem_tog01_hit),
                          &(head->funit->stat->mem_tog10_hit) );
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

}

/*!
 \param ofile    Pointer to output stream to display report information to.

 Generates generic report header for all reports.
*/
void report_print_header( FILE* ofile ) { PROFILE(REPORT_PRINT_HEADER);

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

  if( report_instance ) {
    fprintf( ofile, "* Reported by                    : Instance\n\n" );
  } else {
    fprintf( ofile, "* Reported by                    : Module\n\n" );
  }

  if( (info_suppl.part.excl_assign == 1) ||
      (info_suppl.part.excl_always == 1) ||
      (info_suppl.part.excl_init   == 1) ||
      (info_suppl.part.excl_final  == 1) ||
      (info_suppl.part.excl_pragma == 1) ) {
    fprintf( ofile, "* CDD file excludes the following block types:\n" );
    if( info_suppl.part.excl_assign == 1 ) {
      fprintf( ofile, "    assign - Continuous Assigments\n" );
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

  if( merge_in_num > 0 ) {

    if( merge_in_num == 1 ) {

      fprintf( ofile, "* Report generated from CDD file that was merged from the following files with the following leading hierarchies:\n" );
      fprintf( ofile, "    Filename                                           Leading Hierarchy\n" );
      fprintf( ofile, "    -----------------------------------------------------------------------------------------------------------------\n" );
      fprintf( ofile, "    %-49.49s  %-62.62s\n", input_db,    leading_hierarchies[0] );
      fprintf( ofile, "    %-49.49s  %-62.62s\n", merge_in[0], leading_hierarchies[1] ); 

      if( report_instance && leading_hiers_differ ) {
        fprintf( ofile, "\n* Merged CDD files contain different leading hierarchies, will use value \"<NA>\" to represent leading hierarchy.\n\n" );
      }

    } else {

      fprintf( ofile, "* Report generated from CDD file that was merged from the following files:\n" );
      fprintf( ofile, "    Filename                                           Leading Hierarchy\n" );
      fprintf( ofile, "    -----------------------------------------------------------------------------------------------------------------\n" );

      for( i=0; i<merge_in_num; i++ ) {
        fprintf( ofile, "    %-49.49s  %-62.62s\n", merge_in[i], leading_hierarchies[i+1] );
      }

      if( report_instance && leading_hiers_differ ) {
        fprintf( ofile, "\n* Merged CDD files contain different leading hierarchies, will use value \"<NA>\" to represent leading hierarchy.\n\n" );
      }

    }

    fprintf( ofile, "\n" );

  }

}

/*!
 \param ofile  Pointer to output stream to display report information to.

 Generates a coverage report based on the options specified on the command line
 to the specified output stream.
*/
static void report_generate( FILE* ofile ) { PROFILE(REPORT_GENERATE);

  report_print_header( ofile );

  /* Gather statistics first */
  if( report_instance ) {
    inst_link* instl = inst_head;
    while( instl != NULL ) {
      report_gather_instance_stats( instl->inst );
      instl = instl->next;
    }
  } else {
    report_gather_funit_stats( funit_head );
  }

  /* Call out the proper reports for the specified metrics to report */
  if( report_line ) {
    line_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

  if( report_toggle ) {
    toggle_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

  if( report_memory ) {
    memory_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

  if( report_combination ) {
    combination_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

  if( report_fsm ) {
    fsm_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

  if( report_assertion ) {
    assertion_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

  if( report_race ) {
    race_report( ofile, (report_comb_depth != REPORT_SUMMARY) );
  }

}

/*!
 \param ifile      Name of CDD file to read from.
 \param read_mode  Specifies mode to read from CDD file (merge or replace).
 
 \return Returns TRUE if CDD file was read properly; otherwise, returns FALSE.

 Reads in specified CDD file and gathers functional unit statistics to get ready for GUI
 interaction with this CDD file. 
*/
void report_read_cdd_and_ready( char* ifile, int read_mode ) { PROFILE(REPORT_READ_CDD_AND_READY);

  /* Open database file for reading */
  if( (ifile == NULL) || (ifile[0] == '\0') ) {

    print_output( "CDD file name was not specified for reading", FATAL, __FILE__, __LINE__ );
    Throw 0;

  } else {

    db_read( ifile, read_mode );
    bind_perform( TRUE, 0 );
    report_gather_funit_stats( funit_head );

  }

}

/*!
 \return Returns TRUE if CDD file was closed properly; otherwise, returns FALSE.

 Closes the currently loaded CDD file.
*/
bool report_close_cdd() { PROFILE(REPORT_CLOSE_CDD);

  db_close();

  return( TRUE );

}

/*!
 \param filename  Name to use for saving the currently loaded filename

 \return Returns TRUE if CDD file was saved without error; otherwise, returns FALSE.

 Saves the currently loaded CDD database to the given filename.
*/
bool report_save_cdd( char* filename ) { PROFILE(REPORT_SAVE_CDD);

  return( db_write( filename, FALSE, TRUE ) );

}

/*!
 \param argc      Number of arguments in report command-line.
 \param last_arg  Index of last parsed argument from list.
 \param argv      Arguments passed to report command to parse.

 Performs report command functionality.
*/
void command_report( int argc, int last_arg, const char** argv ) { PROFILE(COMMAND_REPORT);

  FILE*        ofile;                  /* Pointer to output stream */
#ifdef HAVE_TCLTK
  char*        covered_home;           /* Pathname to Covered's home installation directory */
  char*        covered_browser;        /* Name of browser to use for GUI help pages */
  char*        covered_version;        /* String version of current Covered version */
  char*        main_file;              /* Name of main TCL file to interpret */ 
  char*        user_home;              /* HOME environment variable */
#endif
  unsigned int rv;                     /* Return value from snprintf calls */

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, NORMAL, __FILE__, __LINE__ );

  Try {

    /* Parse score command-line */
    report_parse_args( argc, last_arg, argv );

    /* Initialize all global variables */
    info_initialize();

    if( !report_gui ) {

      /* Open database file for reading */
      if( input_db == NULL ) {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Database file not specified in command line" );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( EXIT_FAILURE );

      } else {

        /* Read in CDD file */
        db_read( input_db, (report_instance ? READ_MODE_REPORT_NO_MERGE : READ_MODE_REPORT_MOD_MERGE) );

        /* Perform binding */
        bind_perform( TRUE, 0 );

        Try {

          /* Open output stream */
          if( output_file != NULL ) {
            ofile = fopen( output_file, "w" );
            free_safe( output_file );
            if( ofile == NULL ) {
              unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open report output file %s for writing", output_file );
              assert( rv < USER_MSG_LENGTH );
              print_output( user_msg, FATAL, __FILE__, __LINE__ );
              Throw 0;
            }
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

      Try {

        unsigned int slen;
        unsigned int rv;

        if( input_db != NULL ) {
          report_read_cdd_and_ready( input_db, READ_MODE_REPORT_MOD_MERGE );
        }

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
        covered_browser = getenv( "COVERED_BROWSER" );
#else
        covered_browser = strdup_safe( COVERED_BROWSER );
#endif

        covered_version = strdup_safe( VERSION );
        user_home       = getenv( "HOME" );

        /* Initialize TCL */
        tcl_func_initialize( interp, user_home, covered_home, covered_version, covered_browser );

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
        free_safe( covered_home );
        free_safe( main_file );
        Throw 0;
      }

      /* Clean Up */
      free( covered_home );
      free( main_file );
#endif

    }

  } Catch_anonymous {
    free_safe( input_db );
    db_close();
    Throw 0;
  }

  free_safe( input_db );

  /* Close the database */
  db_close();

  PROFILE_END;

}


/*
 $Log$
 Revision 1.94  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.93  2008/02/06 00:06:04  phase1geo
 Changing metric mode selection from radio buttons to a tk_optionMenu widget.
 Also attempted to fix issue with bad Tcl/Tk code being read in via report
 command.  Still some work to do here, I believe.

 Revision 1.92  2008/02/01 06:37:08  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.91  2008/01/21 21:39:55  phase1geo
 Bug fix for bug 1876376.

 Revision 1.90  2008/01/16 23:10:33  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.89  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.88  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.87  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.86  2008/01/04 23:01:42  phase1geo
 Adding initial form of splint run script.  Started cleaning up splint errors/warnings.

 Revision 1.85  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.84  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.83  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.82  2007/09/01 05:33:38  phase1geo
 Fixing error output in report.c when Tcl/Tk initialization routines return
 in error.  Also removing extra K_EG from static_parser.y file.

 Revision 1.81  2007/04/02 04:50:04  phase1geo
 Adding func_iter files to iterate through a functional unit for reporting
 purposes.  Updated affected files.

 Revision 1.80  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.79  2007/03/13 22:12:59  phase1geo
 Merging changes to covered-0_5-branch to fix bug 1678931.

 Revision 1.78  2006/12/19 05:23:39  phase1geo
 Added initial code for handling instance flattening for unnamed scopes.  This
 is partially working at this point but still needs some debugging.  Checkpointing.

 Revision 1.77.2.1  2007/03/13 22:05:10  phase1geo
 Fixing bug 1678931.  Updated regression.

 Revision 1.77  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.76  2006/10/02 22:41:00  phase1geo
 Lots of bug fixes to memory coverage functionality for GUI.  Memory coverage
 should now be working correctly.  We just need to update the GUI documentation
 as well as other images for the new feature add.

 Revision 1.75  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.74  2006/09/25 04:15:04  phase1geo
 Starting to add support for new memory coverage metric.  This includes changes
 for the report command only at this point.

 Revision 1.73  2006/09/13 23:05:56  phase1geo
 Continuing from last submission.

 Revision 1.72  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.71  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.70  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.69  2006/08/18 04:41:14  phase1geo
 Incorporating bug fixes 1538920 and 1541944.  Updated regressions.  Only
 event1.1 does not currently pass (this does not pass in the stable version
 yet either).

 Revision 1.68  2006/08/02 22:28:32  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.67  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.66  2006/06/20 22:14:32  phase1geo
 Adding support for saving CDD files (needed for file merging and saving exclusion
 information for a CDD file) in the GUI.  Still have a bit to go as I am getting core
 dumps to occur.

 Revision 1.65  2006/06/16 22:44:19  phase1geo
 Beginning to add ability to open/close CDD files without needing to close Covered's
 GUI.  This seems to work but does cause some segfaults yet.

 Revision 1.64  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.63  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.62  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

 Revision 1.61  2006/04/12 18:06:24  phase1geo
 Updating regressions for changes that were made to support multi-file merging.
 Also fixing output of FSM state transitions to be what they were.
 Regressions now pass; however, the support for multi-file merging (beyond two
 files) has not been tested to this point.

 Revision 1.60  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.59  2006/04/07 03:47:50  phase1geo
 Fixing run-time issues with VPI.  Things are running correctly now with IV.

 Revision 1.58  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.57  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.56  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.55  2006/03/20 16:43:38  phase1geo
 Fixing code generator to properly display expressions based on lines.  Regression
 still needs to be updated for these changes.

 Revision 1.54  2006/01/28 06:42:53  phase1geo
 Added configuration read/write functionality for tool preferences and integrated
 the preferences.tcl file into Covered's GUI.  This is now functioning correctly.

 Revision 1.53  2006/01/27 15:43:58  phase1geo
 Added ifdefs for HAVE_ZLIB define to allow Covered to compile correctly when
 zlib.h and associated library is unavailable.  Also handle dumpfile reading
 appropriately for this condition.  Moved report file opening after the CDD file
 has been read in to avoid empty report files when a problem is detected in the
 CDD file.

 Revision 1.52  2006/01/19 00:01:09  phase1geo
 Lots of changes/additions.  Summary report window work is now complete (with the
 exception of adding extra features).  Added support for parsing left and right
 shift operators and the exponent operator in static expression scenarios.  Fixed
 issues related to GUI (due to recent changes in the score command).  Things seem
 to be generally working as expected with the GUI now.

 Revision 1.51  2006/01/06 23:39:10  phase1geo
 Started working on removing the need to simulate more than is necessary.  Things
 are pretty broken at this point, but all of the code should be in -- debugging.

 Revision 1.50  2006/01/03 22:59:16  phase1geo
 Fixing bug in expression_assign function -- removed recursive assignment when
 the LHS expression is a signal, single-bit, multi-bit or static value (only
 recurse when the LHS is a CONCAT or LIST).  Fixing bug in db_close function to
 check if the instance tree has been populated before deallocating memory for it.
 Fixing bug in report help information when Tcl/Tk is not available.  Added bassign2
 diagnostic to regression suite to verify first described bug.

 Revision 1.49  2005/12/13 23:15:15  phase1geo
 More fixes for memory leaks.  Regression fully passes at this point.

 Revision 1.48  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.47  2005/12/12 03:46:14  phase1geo
 Adding exclusion to score command to improve performance.  Updated regression
 which now fully passes.

 Revision 1.46  2005/12/01 19:46:50  phase1geo
 Removed Tcl/Tk from source files if HAVE_TCLTK is not defined.

 Revision 1.45  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.44  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.43  2005/02/05 06:47:20  phase1geo
 Fixing bug with race condition output in instance reporting.  Changing default
 report output of race conditions to false and updating in-line documentation.
 Added rules in regression runs for specifically testing race condition output.
 Updated regression files for these changes.  Regression runs clean at this point.
 We just need to add user documentation for the race condition feature add and
 we should be done.

 Revision 1.42  2005/02/05 06:21:02  phase1geo
 Added ascii report output for race conditions.  There is a segmentation fault
 bug associated with instance reporting.  Need to look into further.

 Revision 1.41  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.40  2004/09/14 19:26:28  phase1geo
 Fixing browser and version injection to Tcl scripts.

 Revision 1.39  2004/09/14 04:54:58  phase1geo
 Adding check for browser to configuration build scripts.  Adding code to set
 BROWSER global variable in Tcl scripts.

 Revision 1.38  2004/08/13 20:45:05  phase1geo
 More added for combinational logic verbose reporting.  At this point, the
 code is being output with underlines that can move up and down the expression
 tree.  No expression reporting is being done at this time, however.

 Revision 1.37  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.36  2004/04/21 05:14:03  phase1geo
 Adding report_gui checking to print_output and adding error handler to TCL
 scripts.  Any errors occurring within the code will be propagated to the user.

 Revision 1.35  2004/04/17 14:07:55  phase1geo
 Adding replace and merge options to file menu.

 Revision 1.34  2004/03/26 13:20:33  phase1geo
 Fixing case where user hits cancel button in open window so that we don't
 exit the GUI when this occurs.

 Revision 1.33  2004/03/25 14:37:07  phase1geo
 Fixing installation of TCL scripts and usage of the scripts in their installed
 location.  We are almost ready to create the new development release.

 Revision 1.32  2004/03/22 13:26:52  phase1geo
 Updates for upcoming release.  We are not quite ready to release at this point.

 Revision 1.31  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.30  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.29  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.28  2004/01/30 23:23:27  phase1geo
 More report output improvements.  Still not ready with regressions.

 Revision 1.27  2004/01/21 22:26:56  phase1geo
 Changed default CDD file name from "cov.db" to "cov.cdd".  Changed instance
 statistic gathering from a child merging algorithm to just calculating
 instance coverage for the individual instances.  Updated full regression for
 this change and updated VCS regression for all past changes of this release.

 Revision 1.26  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.25  2003/12/16 23:22:07  phase1geo
 Adding initial code for line width specification for report output.

 Revision 1.24  2003/11/22 20:44:58  phase1geo
 Adding function to get array of missed line numbers for GUI purposes.  Updates
 to report command for getting information ready when running the GUI.

 Revision 1.23  2003/11/01 01:30:12  phase1geo
 Adding the -view option to the report command parser.  At the current time,
 this option will display an error message to standard error if it is found
 on the command-line.

 Revision 1.22  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.21  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.20  2003/02/18 20:17:03  phase1geo
 Making use of scored flag in CDD file.  Causing report command to exit early
 if it is working on a CDD file which has not been scored.  Updated testsuite
 for these changes.

 Revision 1.19  2002/12/29 06:09:32  phase1geo
 Fixing bug where output was not squelched in report command when -Q option
 is specified.  Fixed bug in preprocessor where spaces where added in when newlines
 found in C-style comment blocks.  Modified regression run to check CDD file and
 generated module and instance reports.  Started to add code to handle signals that
 are specified in design but unused in Covered.

 Revision 1.18  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.17  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.16  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.15  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.14  2002/09/12 05:16:25  phase1geo
 Updating all CDD files in regression suite due to change in vector handling.
 Modified vectors to assign a default value of 0xaa to unassigned registers
 to eliminate bugs where values never assigned and VCD file doesn't contain
 information for these.  Added initial working version of depth feature in
 report generation.  Updates to man page and parameter documentation.

 Revision 1.13  2002/08/20 05:55:25  phase1geo
 Starting to add combination depth option to report command.  Currently, the
 option is not implemented.

 Revision 1.12  2002/08/20 04:48:18  phase1geo
 Adding option to report command that allows the user to display logic that is
 being covered (-c option).  This overrides the default behavior of displaying
 uncovered logic.  This is useful for debugging purposes and understanding what
 logic the tool is capable of handling.

 Revision 1.11  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.10  2002/07/20 18:46:38  phase1geo
 Causing fully covered modules to not be output in reports.  Adding
 instance3.v diagnostic to verify this works correctly.

 Revision 1.9  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.8  2002/07/08 16:06:33  phase1geo
 Updating help information.

 Revision 1.7  2002/07/02 22:37:35  phase1geo
 Changing on-line help command calling.  Regenerated documentation.

 Revision 1.6  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.5  2002/06/25 21:46:10  phase1geo
 Fixes to simulator and reporting.  Still some bugs here.

 Revision 1.4  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.3  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

