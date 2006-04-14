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
 \file     score.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/29/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <sys/times.h>

#include "defines.h"
#include "score.h"
#include "util.h"
#include "link.h"
#include "search.h"
#include "parse.h"
#include "param.h"
#include "vector.h"
#include "fsm_arg.h"
#include "fsm_var.h"
#include "info.h"
#include "perf.h"
#include "vpi.h"


char* top_module                = NULL;                /*!< Name of top-level module to score */
char* top_instance              = NULL;                /*!< Name of top-level instance name */
char* output_db                 = NULL;                /*!< Name of output score database file to generate */
char* dump_file                 = NULL;                /*!< Name of dumpfile to parse */
int   dump_mode                 = DUMP_FMT_NONE;       /*!< Specifies dumpfile format to parse */
char* lxt_file                  = NULL;                /*!< Name of LXT dumpfile to parse */
char* vpi_file                  = NULL;                /*!< Name of VPI output file to write contents to */
int   delay_expr_type           = DELAY_EXPR_DEFAULT;  /*!< Value to use when a delay expression with min:typ:max */
char* ppfilename                = NULL;                /*!< Name of preprocessor filename to use */
bool  instance_specified        = FALSE;               /*!< Specifies if -i option was specified */
int   timestep_update           = 0;                   /*!< Specifies timestep increment to display current time */
int   flag_race_check           = WARNING;             /*!< Specifies how race conditions should be handled */
bool  flag_display_sim_stats    = FALSE;               /*!< Specifies if simulation performance information should be output */

extern unsigned long largest_malloc_size;
extern unsigned long curr_malloc_size;
extern str_link*     use_files_head;
extern char          user_msg[USER_MSG_LENGTH];
extern char*         directive_filename;
extern bool          debug_mode;
extern isuppl        info_suppl;


void define_macro( const char* name, const char* value );


/*!
 Displays usage information for score command.
*/
void score_usage() {

  printf( "\n" );
  printf( "Usage:  covered score -t <top-level_module_name> [-vcd <dumpfile> | -lxt <dumpfile>] [<options>]\n" );
  printf( "\n" );
  printf( "   Dumpfile formats:\n" );
  printf( "      Both the VCD and LXT style dumpfiles are supported by Covered.\n" );
  printf( "\n" );
  printf( "      If either the -vcd or -lxt option is specified, the design is scored using this dumpfile\n" );
  printf( "      for coverage gathering.  If neither option is specified, Covered will only create an\n" );
  printf( "      initial CDD file from the design and will not attempt to score the design.  An error message\n" );
  printf( "      will be displayed if both options are present on the command-line.\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -vpi (<name>)                Generates Verilog module called <name> which contains code to\n" );
  printf( "                                    allow Covered to run as a VPI during simulation.  If <name>\n" );
  printf( "                                    is not specified, the module file is called %s\n", DFLT_VPI_NAME );
  printf( "                                    If the -vcd option is specified along with this option, this\n" );
  printf( "                                    option will not be used.\n" );
  printf( "      -i <instance_name>           Verilog hierarchical scope of top-level module to score.\n" );
  printf( "                                    Necessary if module to verify coverage is not the top-level\n" );
  printf( "                                    module in the design.  If not specified, -t value is used.\n" );
  printf( "      -o <database_filename>       Name of database to write coverage information to.\n" );
  printf( "      -I <directory>               Directory to find included Verilog files.\n" );
  printf( "      -f <filename>                Name of file containing additional arguments to parse.\n" );
  printf( "      -F <module>=(<ivar>,)<ovar>  Module, input state variable and output state variable of\n" );
  printf( "                                    an FSM state variable.  If input variable (ivar) is not specified,\n" );
  printf( "                                    the output variable (ovar) is also used as the input variable.\n" ); 
  printf( "      -a ovl                       Specifies that any OVL assertion found in the design should be\n" );
  printf( "                                    automatically included for assertion coverage.\n" );
#ifdef TBD
  printf( "      -A <module>=<expression>     Module and assertion expression to check for in the design.\n" );
#endif
  printf( "      -y <directory>               Directory to find unspecified Verilog files.\n" );
  printf( "      -v <filename>                Name of specific Verilog file to score.\n" );
  printf( "      -D <define_name>(=<value>)   Defines the specified name to 1 or the specified value.\n" );
  printf( "      -p <filename>                Specifies name of file to use for preprocessor output.\n" );
  printf( "      -P <parameter_scope>=<value> Performs a defparam on the specified parameter with value.\n" );
  printf( "      -T min|typ|max               Specifies value to use in delay expressions of the form min:typ:max.\n" );
  printf( "      -ts <number>                 If design is being scored, specifying this option will output\n" );
  printf( "                                    the current timestep (by increments of <number>) to standard output.\n" );
  printf( "      -r(S|W|E)                    Specifies action to take when race condition checking finds problems in design.\n" );
  printf( "                                    (-rS = Silent.  Do not report condition was found, just handle it.\n" );
  printf( "                                     -rW = Warning.  Report race condition information, but just handle it.  Default.\n" );
  printf( "                                     -rE = Error.  Report race condition information and stop scoring.)\n" );
  printf( "      -S                           Outputs simulation performance information after scoring has completed.  This\n" );
  printf( "                                    information is currently only useful for the developers of Covered.\n" );
  printf( "      -h                           Displays this help information.\n" );
  printf( "\n" );
  printf( "      +libext+.<extension>(+.<extension>)+\n" );
  printf( "                                   Extensions of Verilog files to allow in scoring\n" );
  printf( "\n" );
  printf( "   Optimization Options:\n" );
  printf( "      -e <block_name>              Name of module, task, function or named begin/end block to not score.\n" );
  printf( "      -ec                          Exclude continuous assignment blocks from coverage.\n" );
  printf( "      -ea                          Exclude always blocks from coverage.\n" );
  printf( "      -ei                          Exclude initial blocks from coverage.\n" );
  printf( "    Note:\n" );
  printf( "      The top-level module specifies the module to begin scoring.  All\n" );
  printf( "      modules beneath this module in the hierarchy will also be scored\n" );
  printf( "      unless these modules are explicitly stated to not be scored using\n" );
  printf( "      the -e flag.\n" );
  printf( "\n" );

}

/*!
 \param vpi_file   Name of VPI module to create
 \param output_db  Name of output CDD database file
 \param top_inst   Name of top-level instance

 Creates a Verilog file that calls the Covered VPI system task.
*/
void score_generate_top_vpi_module( char* vpi_file, char* output_db, char* top_inst ) {

  FILE* vfile;     /* File handle to VPI top-level module */
  char* mod_name;  /* Name of VPI module */
  char* ext;       /* Extension of VPI module */

  /* Extract the name of the module from the given filename */
  mod_name = strdup_safe( vpi_file, __FILE__, __LINE__ );
  ext      = strdup_safe( vpi_file, __FILE__, __LINE__ );
  scope_extract_front( vpi_file, mod_name, ext );

  if( ext[0] != '\0' ) {

    if( (vfile = fopen( vpi_file, "w" )) != NULL ) {
  
      fprintf( vfile, "`timescale 1 ns / 1 ps\nmodule %s;\ninitial $covered_sim( \"%s\", %s );\nendmodule\n", mod_name, output_db, top_inst );
      fclose( vfile );

    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open %s for writing", vpi_file );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );

    }

  } else {

    print_output( "Specified -vpi filename did not contain a file extension", FATAL, __FILE__, __LINE__ );
    exit( 1 );

  }

  /* Deallocate memory */
  free_safe( mod_name );
  free_safe( ext );

}

/*!
 \param tab_file  Name of PLI tab file to create
 \param top_mod   Name of top-level module

 Creates a PLI table file.
*/
void score_generate_pli_tab_file( char* tab_file, char* top_mod ) {

  FILE* tfile;     /* File handle of VPI tab file - only necessary for VCS */
  char* mod_name;  /* Name of VPI module */
  char* ext;       /* Extension of VPI module */

  /* Extract the name of the module from the given filename */
  mod_name = (char*)malloc_safe( strlen( tab_file ) + 5, __FILE__, __LINE__ );
  ext      = strdup_safe( tab_file, __FILE__, __LINE__ );
  scope_extract_front( tab_file, mod_name, ext );

  if( ext[0] != '\0' ) {

    strcat( mod_name, ".tab" );
    if( (tfile = fopen( mod_name, "w" )) != NULL ) {

      fprintf( tfile, "$covered_sim  call=covered_sim_calltf  acc+=r,cbk:%s+\n", top_mod );
      fclose( tfile );

    } else {
  
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open %s for writing", mod_name );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );

    }

  } else {

    print_output( "Specified -vpi filename did not contain a file extension", FATAL, __FILE__, __LINE__ );
    exit( 1 );

  }

  /* Deallocate memory */
  free_safe( mod_name );
  free_safe( ext );

}

/*!
 \param cmd_file Name of file to read commands from.
 \param arg_list List of arguments found in specified command file.
 \param arg_num  Number of arguments in arg_list array.
 \return Returns TRUE if read of command file was successful; otherwise,
         returns FALSE.
*/
bool read_command_file( char* cmd_file, char*** arg_list, int* arg_num ) {

  bool      retval  = TRUE;  /* Return value for this function */
  str_link* head    = NULL;  /* Pointer to head element of arg list */
  str_link* tail    = NULL;  /* Pointer to tail element of arg list */
  FILE*     cmd_handle;      /* Pointer to command file */
  char      tmp_str[1024];   /* Temporary holder for read argument */
  str_link* curr;            /* Pointer to current str_link element */
  int       tmp_num = 0;     /* Temporary argument number holder */

  if( file_exists( cmd_file ) ) {

    if( (cmd_handle = fopen( cmd_file, "r" )) != NULL ) {

      while( fscanf( cmd_handle, "%s", tmp_str ) == 1 ) {
        str_link_add( strdup_safe( tmp_str, __FILE__, __LINE__ ), &head, &tail );
        tmp_num++;
      }

      fclose( cmd_handle );

      /* Set the argument list number now */
      *arg_num = tmp_num;

      /*
       If there were any arguments found in the file, create an argument list and pass it to the
       command-line parser.
      */
      if( tmp_num > 0 ) {

        /* Create argument list */
        *arg_list = (char**)malloc_safe( (sizeof( char* ) * tmp_num), __FILE__, __LINE__ );
        tmp_num   = 0;

        curr = head;
        while( curr != NULL ) {
          (*arg_list)[tmp_num] = strdup_safe( curr->str, __FILE__, __LINE__ );
          tmp_num++;
          curr = curr->next;
        }

        /* Delete list */
        str_link_delete_list( head );

      }

    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open command file %s for reading", cmd_file );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = FALSE;

    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Command file %s does not exist", cmd_file );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param argc      Number of arguments specified in argv parameter list.
 \param last_arg  Index of last parsed argument in list.
 \param argv      List of arguments to parse.

 \return Returns TRUE if successful in dealing with arguments; otherwise,
         returns FALSE.

 Parses score command argument list and performs specified functions based
 on these arguments.
*/
bool score_parse_args( int argc, int last_arg, char** argv ) {

  bool   retval  = TRUE;          /* Return value for this function */
  int    i       = last_arg + 1;  /* Loop iterator */
  char** arg_list;                /* List of command_line arguments */
  int    arg_num;                 /* Number of arguments in arg_list */
  int    j;                       /* Loop iterator */
  char*  ptr;                     /* Pointer to current character in defined value */

  while( (i < argc) && retval ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      score_usage();
      retval = FALSE;

    } else if( strncmp( "-i", argv[i], 2 ) == 0 ) {

      i++;
      top_instance       = strdup_safe( argv[i], __FILE__, __LINE__ );
      instance_specified = TRUE;

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      i++;
      if( is_directory( argv[i] ) ) {
        output_db = strdup_safe( argv[i], __FILE__, __LINE__ );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Illegal output directory specified \"%s\"", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else if( strncmp( "-ts", argv[i], 3 ) == 0 ) {

      i++;
      timestep_update = atol( argv[i] );

    } else if( strncmp( "-t", argv[i], 2 ) == 0 ) {

      i++;
      if( is_variable( argv[i] ) ) {
        top_module = strdup_safe( argv[i], __FILE__, __LINE__ );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Illegal top-level module name specified \"%s\"", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else if( strncmp( "-I", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_include_path( argv[i] );

    } else if( strncmp( "-y", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_directory_path( argv[i] );

    } else if( strncmp( "-F", argv[i], 2 ) == 0 ) {

      i++;
      retval = fsm_arg_parse( argv[i] );
      
    } else if( strncmp( "-f", argv[i], 2 ) == 0 ) {

      i++;
      if( file_exists( argv[i] ) ) {
        read_command_file( argv[i], &arg_list, &arg_num );
        retval = score_parse_args( arg_num, -1, arg_list );
        for( j=0; j<arg_num; j++ ) {
          free_safe( arg_list[j] );
        }
        free_safe( arg_list );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Cannot find argument file %s specified with -f option", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else if( strncmp( "-ec", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_assign = 1;

    } else if( strncmp( "-ea", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_always = 1;

    } else if( strncmp( "-ei", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_init = 1;

    } else if( strncmp( "-e", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_no_score_funit( argv[i] );

    } else if( strncmp( "-vcd", argv[i], 4 ) == 0 ) {

      i++;
      if( dump_mode == DUMP_FMT_NONE ) {
        if( file_exists( argv[i] ) ) {
          dump_file = strdup_safe( argv[i], __FILE__, __LINE__ );
          dump_mode = DUMP_FMT_VCD;
        } else {
          snprintf( user_msg, USER_MSG_LENGTH, "VCD dumpfile not found \"%s\"", argv[i] );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          retval = FALSE;
        }
      } else {
        print_output( "Both the -vcd and -lxt options were specified on the command-line", FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else if( strncmp( "-lxt", argv[i], 4 ) == 0 ) {
 
      i++; 
      if( dump_mode == DUMP_FMT_NONE ) {
        if( file_exists( argv[i] ) ) {
          dump_file = strdup_safe( argv[i], __FILE__, __LINE__ );
          dump_mode = DUMP_FMT_LXT;
        } else {
          snprintf( user_msg, USER_MSG_LENGTH, "LXT dumpfile not found \"%s\"", argv[i] );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          retval = FALSE;
        }
      } else {
        print_output( "Both the -vcd and -lxt options were specified on the command-line", FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else if( strncmp( "-vpi", argv[i], 4 ) == 0 ) {

      i++;
      if( (i < argc) && (argv[i][0] != '-') ) {
        vpi_file = strdup_safe( argv[i], __FILE__, __LINE__ );
      } else {
        vpi_file = strdup_safe( DFLT_VPI_NAME, __FILE__, __LINE__ );
        i--;
      }

    } else if( strncmp( "-v", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_file( argv[i] );

    } else if( strncmp( "+libext+", argv[i], 8 ) == 0 ) {

      retval = search_add_extensions( argv[i] + 8 );

    } else if( strncmp( "-D", argv[i], 2 ) == 0 ) {

      i++;
      ptr = argv[i];
      while( (*ptr != '\0') && (*ptr != '=') ) {
        ptr++;
      }
      if( *ptr == '=' ) {
        *ptr = '\0';
        ptr++;
        define_macro( argv[i], ptr );
      } else {
        define_macro( argv[i], "1" );
      }

    } else if( strncmp( "-p", argv[i], 2 ) == 0 ) {
      
      i++;
      if( is_variable( argv[i] ) ) {
        ppfilename = strdup_safe( argv[i], __FILE__, __LINE__ );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Unrecognizable filename %s specified for -p option.", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      }
        
    } else if( strncmp( "-P", argv[i], 2 ) == 0 ) {

      i++;
      ptr = argv[i];
      while( (*ptr != '\0') && (*ptr != '=') ) {
        ptr++;
      }
      if( *ptr == '\0' ) {
        print_output( "Option -P must specify a value to assign.  See \"covered score -h\" for more information.",
                      FATAL, __FILE__, __LINE__ );
        exit( 1 );
      } else {
        *ptr = '\0';
        ptr++;
        defparam_add( argv[i], vector_from_string( &ptr, FALSE ) );
      }
      
    } else if( strncmp( "-T", argv[i], 2 ) == 0 ) {
      
      i++;
      
      if( strcmp( argv[i], "min" ) == 0 ) {
        delay_expr_type = DELAY_EXPR_MIN;
      } else if( strcmp( argv[i], "max" ) == 0 ) {
        delay_expr_type = DELAY_EXPR_MAX;
      } else if( strcmp( argv[i], "typ" ) == 0 ) {
        delay_expr_type = DELAY_EXPR_TYP;
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Unknown -T value (%s).  Please specify min, max or typ.", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      }

    } else if( strncmp( "-r", argv[i], 2 ) == 0 ) {

      switch( argv[i][2] ) {
        case 'E'  :  flag_race_check = FATAL;    break;
        case 'W'  :  flag_race_check = WARNING;  break;
        case 'S'  :
        case '\0' :  flag_race_check = NORMAL;   break;
        default   :
          snprintf( user_msg, USER_MSG_LENGTH, "Unknown race condition value %c (available types are E, W or S)", argv[i][2] );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          retval = FALSE;
          break;
      }

    } else if( strncmp( "-S", argv[i], 2 ) == 0 ) {

      flag_display_sim_stats = TRUE;

    } else if( strncmp( "-a", argv[i], 2 ) == 0 ) {

      i++;

      if( strncmp( argv[i], "ovl", 3 ) == 0 ) {
        info_suppl.part.assert_ovl = 1;
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Unknown -a value (%s).  Please specify ovl.", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }
        
    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Unknown score command option \"%s\".  See \"covered score -h\" for more information.", argv[i] );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = FALSE;

    }

    i++;

  }

  return( retval );

}

/*!
 \param argc      Number of arguments in score command-line.
 \param last_arg  Index of last parsed argument in list.
 \param argv      Arguments from command-line to parse.

 \return Returns 0 if scoring is successful; otherwise, returns -1.

 Performs score command functionality.
*/
int command_score( int argc, int last_arg, char** argv ) {

  int retval = 0;  /* Return value for this function */

  /* Parse score command-line */
  if( score_parse_args( argc, last_arg, argv ) ) {

    snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );

    if( output_db == NULL ) {
      output_db = strdup_safe( DFLT_OUTPUT_CDD, __FILE__, __LINE__ );
    }

    /* Parse design */
    if( use_files_head != NULL ) {
      print_output( "Reading design...", NORMAL, __FILE__, __LINE__ );
      info_initialize();
      search_init();
      parse_design( top_module, output_db );
      print_output( "", NORMAL, __FILE__, __LINE__ );
    }

    /* Generate VPI-based top module */
    if( vpi_file != NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Outputting VPI file %s...", vpi_file );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      score_generate_top_vpi_module( vpi_file, output_db, top_instance );
      score_generate_pli_tab_file( vpi_file, top_module );

    /* Read dumpfile and score design */
    } else if( dump_mode != DUMP_FMT_NONE ) {

      switch( dump_mode ) {
        case DUMP_FMT_VCD :  snprintf( user_msg, USER_MSG_LENGTH, "Scoring VCD dumpfile %s...", dump_file );  break;
        case DUMP_FMT_LXT :  snprintf( user_msg, USER_MSG_LENGTH, "Scoring LXT dumpfile %s...", dump_file );  break;
      }
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      parse_and_score_dumpfile( output_db, dump_file, dump_mode );
      print_output( "", NORMAL, __FILE__, __LINE__ );

    }

    if( dump_mode != DUMP_FMT_NONE ) {
      print_output( "***  Scoring completed successfully!  ***\n", NORMAL, __FILE__, __LINE__ );
    }
    snprintf( user_msg, USER_MSG_LENGTH, "Dynamic memory allocated:   %ld bytes", largest_malloc_size );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Allocated memory remaining: %ld bytes", curr_malloc_size );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
    print_output( "", NORMAL, __FILE__, __LINE__ );

    /* Display simulation statistics if specified */
    if( flag_display_sim_stats ) {
      perf_output_inst_report( stdout );
    }

    /* Close database */
    db_close();

    /* Deallocate memory for search engine */
    search_free_lists();

    /* Deallocate memory for defparams */
    defparam_dealloc();

    free_safe( output_db );
    free_safe( dump_file );
    free_safe( vpi_file );
    free_safe( top_module );
    free_safe( ppfilename );
    ppfilename = NULL;

    free_safe( directive_filename );
    free_safe( top_instance );

  }

  return( retval );

}

/*
 $Log$
 Revision 1.69  2006/04/13 22:17:47  phase1geo
 Adding the beginning of the OVL assertion extractor.  So far the -a option is
 parsed and the race condition checker is turned off for all detectable
 OVL assertion modules (we will trust that these modules don't have race conditions
 inherent in them).

 Revision 1.68  2006/04/13 21:04:24  phase1geo
 Adding NOOP expression and allowing $display system calls to not cause its
 statement block to be excluded from coverage.  Updating regressions which fully
 pass.

 Revision 1.67  2006/04/07 22:31:07  phase1geo
 Fixes to get VPI to work with VCS.  Getting close but still some work to go to
 get the callbacks to start working.

 Revision 1.66  2006/04/06 22:30:03  phase1geo
 Adding VPI capability back and integrating into autoconf/automake scheme.  We
 are getting close but still have a ways to go.

 Revision 1.65  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.64  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.63  2006/02/06 15:35:36  phase1geo
 Fixing bug with -f option when the file is empty (previously segfaulted).

 Revision 1.62  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

 Revision 1.61  2006/01/09 18:58:15  phase1geo
 Updating regression for VCS runs.  Added cleanup function at exit to remove the
 tmp* file (if it exists) regardless of the internal state of Covered at the time
 of exit (removes the need for the user to remove this file when things go awry).
 Documentation updates for this feature.

 Revision 1.60  2006/01/04 22:07:04  phase1geo
 Changing expression execution calculation from sim to expression_operate function.
 Updating all regression files for this change.  Modifications to diagnostic Makefile
 to accommodate environments that do not have valgrind.

 Revision 1.59  2006/01/04 03:15:52  phase1geo
 Adding bassign3 diagnostic to regression suite to verify expression_assign
 function works correctly for CONCAT/LIST ordering.

 Revision 1.58  2006/01/02 21:35:36  phase1geo
 Added simulation performance statistical information to end of score command
 when we are in debug mode.

 Revision 1.57  2005/12/31 03:30:47  phase1geo
 Various documentation updates.

 Revision 1.56  2005/12/22 23:04:42  phase1geo
 More memory leak fixes.

 Revision 1.55  2005/12/21 22:30:54  phase1geo
 More updates to memory leak fix list.  We are getting close!  Added some helper
 scripts/rules to more easily debug valgrind memory leak errors.  Also added suppression
 file for valgrind for a memory leak problem that exists in lex-generated code.

 Revision 1.54  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.53  2005/12/12 03:46:14  phase1geo
 Adding exclusion to score command to improve performance.  Updated regression
 which now fully passes.

 Revision 1.52  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.51  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.50  2005/05/09 03:08:35  phase1geo
 Intermediate checkin for VPI changes.  Also contains parser fix which should
 be branch applied to the latest stable and development versions.

 Revision 1.49  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.48  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.47  2005/01/03 23:00:35  phase1geo
 Fixing library extension parser.

 Revision 1.46  2004/12/17 22:29:36  phase1geo
 More code added to race condition feature.  Still not usable.

 Revision 1.45  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.44  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.43  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.42  2004/01/21 22:26:56  phase1geo
 Changed default CDD file name from "cov.db" to "cov.cdd".  Changed instance
 statistic gathering from a child merging algorithm to just calculating
 instance coverage for the individual instances.  Updated full regression for
 this change and updated VCS regression for all past changes of this release.

 Revision 1.41  2003/10/28 13:28:00  phase1geo
 Updates for more FSM attribute handling.  Not quite there yet but full regression
 still passes.

 Revision 1.40  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.39  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.38  2003/09/22 03:46:24  phase1geo
 Adding support for single state variable FSMs.  Allow two different ways to
 specify FSMs on command-line.  Added diagnostics to verify new functionality.

 Revision 1.37  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.36  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.35  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.34  2003/08/07 15:41:43  phase1geo
 Adding -ts option to score command to allow the current timestep to be
 output during the simulation phase.

 Revision 1.33  2003/02/07 23:12:30  phase1geo
 Optimizing db_add_statement function to avoid memory errors.  Adding check
 for -i option to avoid user error.

 Revision 1.32  2003/01/06 00:44:21  phase1geo
 Updates to NEWS, ChangeLog, development documentation and user documentation
 for new 0.2pre1_20030105 release.

 Revision 1.31  2002/11/27 06:03:35  phase1geo
 Adding diagnostics to verify selectable delay.  Removing selectable delay
 warning from being output constantly to only outputting when selectable delay
 found in design and -T option not specified.  Full regression passes.

 Revision 1.30  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.29  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.28  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.27  2002/10/13 19:20:42  phase1geo
 Added -T option to score command for properly handling min:typ:max delay expressions.
 Updated documentation for -i and -T options to score command and added additional
 diagnostic to test instance handling.

 Revision 1.26  2002/10/13 13:55:53  phase1geo
 Fixing instance depth selection and updating all configuration files for
 regression.  Full regression now passes.

 Revision 1.25  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.24  2002/09/27 01:19:38  phase1geo
 Fixed problems with parameter overriding from command-line.  This now works
 and param1.2.v has been added to test this functionality.  Totally reworked
 regression running to allow each diagnostic to specify unique command-line
 arguments to Covered.  Full regression passes.

 Revision 1.23  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.22  2002/09/21 04:11:32  phase1geo
 Completed phase 1 for adding in parameter support.  Main code is written
 that will create an instance parameter from a given module parameter in
 its entirety.  The next step will be to complete the module parameter
 creation code all the way to the parser.  Regression still passes and
 everything compiles at this point.

 Revision 1.21  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.20  2002/08/26 21:31:22  phase1geo
 Updating score help output to reflect previous changes to score command.

 Revision 1.19  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.18  2002/08/19 21:36:25  phase1geo
 Fixing memory corruption bug in score function for adding Verilog modules
 to use_files list.  This caused a core dump to occur when the -f option
 was used.

 Revision 1.17  2002/07/21 00:08:58  phase1geo
 Updating score usage information.  Updated manstyle user documentation though
 there seems to be some problem getting the HTML generated from this.  Getting
 ready for the next release.

 Revision 1.16  2002/07/20 22:22:52  phase1geo
 Added ability to create implicit signals for local signals.  Added implicit1.v
 diagnostic to test for correctness.  Full regression passes.  Other tweaks to
 output information.

 Revision 1.15  2002/07/20 21:34:58  phase1geo
 Separating ability to parse design and score dumpfile.  Now both or either
 can be done (allowing one to parse once and score multiple times).

 Revision 1.14  2002/07/17 12:53:04  phase1geo
 Added -D option to score command and verified that this works properly.

 Revision 1.13  2002/07/17 00:13:57  phase1geo
 Added support for -e option and informally tested.

 Revision 1.12  2002/07/16 03:29:18  phase1geo
 Updates to NEWS, INSTALL, ChangeLog for release.  Modifications to Verilog
 diagnostic Makefile to make it easier to read.  Added user warning if -e
 option is specified since this is not supported at this time.  Removed
 mpatrol from configure.in.

 Revision 1.11  2002/07/11 15:10:00  phase1geo
 Fixing -f option to score command.  This function was causing infinite loops
 and massive memory consumption as a result of this.  Fixes bug 579946.

 Revision 1.10  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.9  2002/07/08 19:02:12  phase1geo
 Adding -i option to properly handle modules specified for coverage that
 are instantiated within a design without needing to parse parent modules.

 Revision 1.8  2002/07/08 16:06:33  phase1geo
 Updating help information.

 Revision 1.7  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.6  2002/07/03 21:30:53  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.5  2002/07/02 22:37:35  phase1geo
 Changing on-line help command calling.  Regenerated documentation.
*/

