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
 \author   Trevor Williams  (phase1geo@gmail.com)
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
#include <unistd.h>

#ifdef DEBUG_MODE
#include "cli.h"
#endif
#include "db.h"
#include "defines.h"
#include "fsm_arg.h"
#include "fsm_var.h"
#include "link.h"
#include "ovl.h"
#include "parse.h"
#include "param.h"
#include "perf.h"
#include "score.h"
#include "search.h"
#include "info.h"
#include "util.h"
#include "vector.h"
#include "vpi.h"


/*! Name of top-level module to score */
char* top_module = NULL;

/*! Name of top-level instance name */
char* top_instance = NULL;

/*! Name of output score database file to generate */
static char* output_db = NULL;

/*! Name of dumpfile to parse */
static char* dump_file = NULL;

/*! Specifies dumpfile format to parse */
static int dump_mode = DUMP_FMT_NONE;

/*! Name of LXT dumpfile to parse */
char* lxt_file = NULL;

/*! Name of VPI output file to write contents to */
static char* vpi_file = NULL;

/*! Value to use when a delay expression with min:typ:max */
int delay_expr_type = DELAY_EXPR_DEFAULT;

/*! Name of preprocessor filename to use */
char* ppfilename = NULL;

/*! Specifies if -i option was specified */
bool instance_specified = FALSE;

/*! Specifies timestep increment to display current time */
uint64 timestep_update = 0;

/*! Specifies how race conditions should be handled */
int flag_race_check = WARNING;

/*! Specifies if race condition checking should occur */
bool flag_check_races = TRUE;

/*! Specifies if simulation performance information should be output */
static bool flag_display_sim_stats = FALSE;

/*! Specifies the supported global generation value */
int flag_global_generation = GENERATION_SV;

/*! Specifies whether the command-line debugger should be enabled */
bool flag_use_command_line_debug = FALSE;

/*! Specifies the name of an input file to use for debugging */
char* command_line_debug_file = NULL;

/*! Pointer to the head of the generation module list */
str_link* gen_mod_head = NULL;

/*! Pointer to the tail of the generation module list */
static str_link* gen_mod_tail = NULL;

/*! Specifies the user-supplied timescale information for VPI */
static char* vpi_timescale = NULL;

extern int64     largest_malloc_size;
extern int64     curr_malloc_size;
extern str_link* use_files_head;
extern char      user_msg[USER_MSG_LENGTH];
extern char*     directive_filename;
extern bool      debug_mode;
extern isuppl    info_suppl;
extern char*     pragma_coverage_name;
extern char*     pragma_racecheck_name;
extern char      score_run_path[4096];
extern char**    score_args;
extern int       score_arg_num;


extern void process_timescale( const char* txt, bool report );
extern void define_macro( const char* name, const char* value );


/*!
 Displays usage information for score command.
*/
static void score_usage() {

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
  printf( "      -vpi_ts <timescale>          This option is only valid when the -vpi option has been specified.\n" );
  printf( "                                    This option allows the user to specify a timescale for the generated\n" );
  printf( "                                    Verilog module.  If this option is not specified, no timescale will\n" );
  printf( "                                    be created for the generated module.  The value of <timescale> is\n" );
  printf( "                                    specified as follows:\n" );
  printf( "                                      (1|10|100)(s|ms|us|ns|ps|fs)/(1|10|100)(s|ms|us|ns|ps|fs)\n" );
  printf( "                                    If whitespace is needed between the various values, place the\n" );
  printf( "                                    entire contents of <timescale> in double quotes.\n" );
  printf( "      -i <instance_name>           Verilog hierarchical scope of top-level module to score.\n" );
  printf( "                                    Necessary if module to verify coverage is not the top-level\n" );
  printf( "                                    module in the design.  If not specified, -t value is used.\n" );
  printf( "      -o <database_filename>       Name of database to write coverage information to.\n" );
  printf( "      -I <directory>               Directory to find included Verilog files.\n" );
  printf( "      -f <filename>                Name of file containing additional arguments to parse.\n" );
  printf( "      -F <module>=(<ivar>,)<ovar>  Module, input state variable and output state variable of\n" );
  printf( "                                    an FSM state variable.  If input variable (ivar) is not specified,\n" );
  printf( "                                    the output variable (ovar) is also used as the input variable.\n" ); 
  printf( "      -A ovl                       Specifies that any OVL assertion found in the design should be\n" );
  printf( "                                    automatically included for assertion coverage.\n" );
  printf( "      -y <directory>               Directory to find unspecified Verilog files.\n" );
  printf( "      -v <filename>                Name of specific Verilog file to score.\n" );
  printf( "      -D <define_name>(=<value>)   Defines the specified name to 1 or the specified value.\n" );
  printf( "      -p <filename>                Specifies name of file to use for preprocessor output.\n" );
  printf( "      -P <parameter_scope>=<value> Performs a defparam on the specified parameter with value.\n" );
  printf( "      -T min|typ|max               Specifies value to use in delay expressions of the form min:typ:max.\n" );
  printf( "      -ts <number>                 If design is being scored, specifying this option will output\n" );
  printf( "                                    the current timestep (by increments of <number>) to standard output.\n" );
  printf( "      -r(S|W|E|I|P[=<name>])       Specifies action to take when race condition checking finds problems in design.\n" );
  printf( "                                    (-rS = Silent.  Do not report condition was found, just handle it.\n" );
  printf( "                                     -rW = Warning.  Report race condition information, but just handle it.  Default.\n" );
  printf( "                                     -rE = Error.  Report race condition information and stop scoring.\n" );
  printf( "                                     -rI = Ignore.  Skip race condition checking completely.)\n" );
  printf( "                                     -rP = Use pragmas.  Skip race condition checking for all code surrounded by\n" );
  printf( "                                           // racecheck off/on embedded pragmas.  The \"racecheck\" keyword can be\n" );
  printf( "                                           changed by specifying =<name> where <name> is the new name for the race\n" );
  printf( "                                           condition pragma keyword.\n" );
  printf( "      -S                           Outputs simulation performance information after scoring has completed.  This\n" );
  printf( "                                    information is currently only useful for the developers of Covered.\n" );
  printf( "      -g (<module>=)[1|2|3]        Selects generation of Verilog syntax that the parser will handle.  If\n" );
  printf( "                                    <module>= is present, only the specified module will use the provided\n" );
  printf( "                                    generation.  If <module>= is not specified, the entire design will use\n" );
  printf( "                                    the provided generation.  1=Verilog-1995, 2=Verilog-2001, 3=SystemVerilog\n" );
  printf( "                                    By default, the latest generation is parsed.\n" );
  printf( "      -cli (<filename>)            Causes the command-line debugger to be used during VCD/LXT dumpfile scoring.\n" );
  printf( "                                    If <filename> is specified, this file contains information saved in a previous\n" );
  printf( "                                    call to savehist on the CLI and causes the history contained in this file to be\n" );
  printf( "                                    replayed prior to the CLI command prompt.  If <filename> is not specified, the\n" );
  printf( "                                    CLI prompt will be immediately available at the start of simulation.  This option\n" );
  printf( "                                    is only available when Covered is configured with the --enable-debug option.\n" );
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
  printf( "      -ef                          Exclude final blocks from coverage.\n" );
  printf( "      -ep [<name>]                 Exclude all code enclosed by pragmas.  By default, the pragmas are of\n" );
  printf( "                                   the format '// coverage (on|off)'; however, if <name> is specified for\n" );
  printf( "                                   this option, the pragma keyword 'coverage' will be replaced with that value.\n" );
  printf( "\n" );
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

 \throws anonymous Throw

 Creates a Verilog file that calls the Covered VPI system task.
*/
static void score_generate_top_vpi_module(
  const char* vpi_file,
  const char* output_db,
  const char* top_inst
) { PROFILE(SCORE_GENERATE_TOP_VPI_MODULE);

  FILE* vfile;     /* File handle to VPI top-level module */
  char* mod_name;  /* Name of VPI module */
  char* ext;       /* Extension of VPI module */

  /* Extract the name of the module from the given filename */
  mod_name = strdup_safe( vpi_file );
  ext      = strdup_safe( vpi_file );
  scope_extract_front( vpi_file, mod_name, ext );

  Try {
 
    if( ext[0] != '\0' ) {

      if( (vfile = fopen( vpi_file, "w" )) != NULL ) {
  
        unsigned int rv;
        if( vpi_timescale != NULL ) {
          fprintf( vfile, "`timescale %s\n", vpi_timescale );
        }
        fprintf( vfile, "module %s;\ninitial $covered_sim( \"%s\", %s );\nendmodule\n", mod_name, output_db, top_inst );
        rv = fclose( vfile );
        assert( rv == 0 );

      } else {
  
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open %s for writing", vpi_file );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "score Throw A\n" );
        Throw 0;

      }

    } else {

      print_output( "Specified -vpi filename did not contain a file extension", FATAL, __FILE__, __LINE__ );
      printf( "score Throw B\n" );
      Throw 0;

    }

  } Catch_anonymous {
    free_safe( mod_name, (strlen( mod_name ) + 1) );
    free_safe( ext, (strlen( ext ) + 1) );
    printf( "score Throw C\n" );
    Throw 0;
  }

  /* Deallocate memory */
  free_safe( mod_name, (strlen( mod_name ) + 1) );
  free_safe( ext, (strlen( ext ) + 1) );

}

/*!
 \param tab_file  Name of PLI tab file to create
 \param top_mod   Name of top-level module

 \throws anonymous Throw

 Creates a PLI table file.
*/
static void score_generate_pli_tab_file(
  const char* tab_file,
  const char* top_mod
) { PROFILE(SCORE_GENERATE_PLI_TAB_FILE);

  FILE* tfile;     /* File handle of VPI tab file - only necessary for VCS */
  char* mod_name;  /* Name of VPI module */
  char* ext;       /* Extension of VPI module */

  /* Extract the name of the module from the given filename */
  mod_name = (char*)malloc_safe( strlen( tab_file ) + 5 );
  ext      = strdup_safe( tab_file );
  scope_extract_front( tab_file, mod_name, ext );

  Try {

    if( ext[0] != '\0' ) {

      strcat( mod_name, ".tab" );
      if( (tfile = fopen( mod_name, "w" )) != NULL ) {

        unsigned int rv;
        fprintf( tfile, "$covered_sim  call=covered_sim_calltf  acc+=r,cbk:%s+\n", top_mod );
        rv = fclose( tfile );
        assert( rv == 0 );

      } else {
  
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open %s for writing", mod_name );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "score Throw D\n" );
        Throw 0;

      }

    } else {

      print_output( "Specified -vpi filename did not contain a file extension", FATAL, __FILE__, __LINE__ );
      printf( "score Throw E\n" );
      Throw 0;

    }

  } Catch_anonymous {
    free_safe( mod_name, (strlen( mod_name ) + 1) );
    free_safe( ext, (strlen( ext ) + 1) );
    printf( "score Throw F\n" );
    Throw 0;
  }

  /* Deallocate memory */
  free_safe( mod_name, (strlen( mod_name ) + 1) );
  free_safe( ext, (strlen( ext ) + 1) );

}

/*!
 \param cmd_file Name of file to read commands from.
 \param arg_list List of arguments found in specified command file.
 \param arg_num  Number of arguments in arg_list array.

 \throws anonymous Throw Throw Throw substitute_env_vars

 Parses the given file specified by the '-f' option to Covered's score command which can contain
 any command-line arguments.
*/
static void read_command_file(
  const char* cmd_file,
  char***     arg_list,
  int*        arg_num
) { PROFILE(READ_COMMAND_FILE);

  str_link* head    = NULL;  /* Pointer to head element of arg list */
  str_link* tail    = NULL;  /* Pointer to tail element of arg list */
  FILE*     cmd_handle;      /* Pointer to command file */
  char      tmp_str[4096];   /* Temporary holder for read argument */
  str_link* curr;            /* Pointer to current str_link element */
  int       tmp_num = 0;     /* Temporary argument number holder */

  if( file_exists( cmd_file ) ) {

    if( (cmd_handle = fopen( cmd_file, "r" )) != NULL ) {

      unsigned int rv;

      Try {

        while( fscanf( cmd_handle, "%s", tmp_str ) == 1 ) {
          (void)str_link_add( substitute_env_vars( tmp_str ), &head, &tail );
          tmp_num++;
        }

      } Catch_anonymous {
        rv = fclose( cmd_handle );
        assert( rv == 0 );
        printf( "score Throw G\n" );
        Throw 0;
      }

      rv = fclose( cmd_handle );
      assert( rv == 0 );

      /* Set the argument list number now */
      *arg_num = tmp_num;

      /*
       If there were any arguments found in the file, create an argument list and pass it to the
       command-line parser.
      */
      if( tmp_num > 0 ) {

        /* Create argument list */
        *arg_list = (char**)malloc_safe( sizeof( char* ) * tmp_num );
        tmp_num   = 0;

        curr = head;
        while( curr != NULL ) {
          (*arg_list)[tmp_num] = strdup_safe( curr->str );
          tmp_num++;
          curr = curr->next;
        }

        /* Delete list */
        str_link_delete_list( head );

      }

    } else {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open command file %s for reading", cmd_file );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      printf( "score Throw H\n" );
      Throw 0;

    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Command file %s does not exist", cmd_file );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    printf( "score Throw I\n" );
    Throw 0;

  }

}

/*!
 \param def  Define value to parse
 
 Parses the specified define from the command-line, storing the define value in the
 define tree according to its value.
*/
void score_parse_define( const char* def ) { PROFILE(SCORE_PARSE_DEFINE);

  char* tmp = strdup_safe( def );  /* Temporary copy of the given argument */
  char* ptr = tmp;                 /* Pointer to current character in define */

  while( (*ptr != '\0') && (*ptr != '=') ) {
    ptr++;
  }

  if( *ptr == '=' ) {
    *ptr = '\0';
    ptr++;
    define_macro( tmp, ptr );
  } else {
    define_macro( tmp, "1" );
  }

  /* Deallocate memory */
  free_safe( tmp, (strlen( tmp ) + 1) );

}

/*!
 \param arg  Argument from score command.
 
 Adds the specified argument to the list of score arguments that will be written to the CDD file.
*/
static void score_add_arg(
  const char* arg
) { PROFILE(SCORE_ADD_ARG);

  score_args = (char**)realloc_safe( score_args, (sizeof( char* ) * score_arg_num), (sizeof( char* ) * (score_arg_num + 1)) );
  score_args[score_arg_num] = strdup_safe( arg );
  score_arg_num++;

}

/*!
 \param argc      Number of arguments specified in argv parameter list.
 \param last_arg  Index of last parsed argument in list.
 \param argv      List of arguments to parse.

 \throws anonymous search_add_directory_path Throw Throw Throw Throw Throw Throw Throw Throw Throw Throw Throw Throw
                   Throw Throw Throw Throw Throw Throw Throw Throw Throw Throw score_parse_args ovl_add_assertions_to_no_score_list
                   fsm_arg_parse read_command_file search_add_file defparam_add search_add_extensions search_add_no_score_funit

 Parses score command argument list and performs specified functions based
 on these arguments.
*/
static void score_parse_args(
  int          argc,
  int          last_arg,
  const char** argv
) { PROFILE(SCORE_PARSE_ARGS);

  int    i = last_arg + 1;  /* Loop iterator */
  char** arg_list;          /* List of command_line arguments */
  int    arg_num;           /* Number of arguments in arg_list */
  int    j;                 /* Loop iterator */
  char*  ptr;               /* Pointer to current character in defined value */
  char*  rv;                /* Return value from snprintf calls */

  while( i < argc ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      score_usage();
      printf( "score Throw J\n" );
      Throw 0;

    } else if( strncmp( "-i", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( instance_specified ) {
          print_output( "Only one -i option may be present on the command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          top_instance       = strdup_safe( argv[i] );
          score_add_arg( argv[i-1] );
          score_add_arg( argv[i] );
          instance_specified = TRUE;
        }
      } else {
        printf( "score Throw K\n" );
        Throw 0;
      }

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( output_db != NULL ) {
          print_output( "Only one -o option may be present on the command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( file_exists( argv[i] ) || is_legal_filename( argv[i] ) ) {
            output_db = strdup_safe( argv[i] );
            score_add_arg( argv[i-1] );
            score_add_arg( argv[i] );
          } else {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output file \"%s\" is not writable", argv[i] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "score Throw L\n" );
            Throw 0;
          }
        }
      } else {
        printf( "score Throw M\n" );
        Throw 0;
      }

    } else if( strncmp( "-ts", argv[i], 3 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( timestep_update != 0 ) {
          print_output( "Only one -ts option may be present on the command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          timestep_update = ato64( argv[i] );
          score_add_arg( argv[i-1] );
          score_add_arg( argv[i] );
        }
      } else {
        printf( "score Throw N\n" );
        Throw 0;
      }

    } else if( strncmp( "-t", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( top_module != NULL ) {
          print_output( "Only one -t option may be present on the command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( is_variable( argv[i] ) ) {
            top_module = strdup_safe( argv[i] );
            score_add_arg( argv[i-1] );
            score_add_arg( argv[i] );
          } else {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal top-level module name specified \"%s\"", argv[i] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "score Throw O\n" );
            Throw 0;
          }
        }
      } else {
        printf( "score Throw P\n" );
        Throw 0;
      }

    } else if( strncmp( "-I", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        search_add_include_path( argv[i] );
        score_add_arg( argv[i-1] );
        score_add_arg( argv[i] );
      } else {
        printf( "score Throw Q\n" );
        Throw 0;
      }

    } else if( strncmp( "-y", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        search_add_directory_path( argv[i] );
        score_add_arg( argv[i-1] );
        score_add_arg( argv[i] );
      } else {
        printf( "score Throw R\n" );
        Throw 0;
      }

    } else if( strncmp( "-F", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        fsm_arg_parse( argv[i] );
        score_add_arg( argv[i-1] );
        score_add_arg( argv[i] );
      } else {
        printf( "score Throw S\n" );
        Throw 0;
      }
      
    } else if( strncmp( "-f", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( file_exists( argv[i] ) ) {
          Try {
            read_command_file( argv[i], &arg_list, &arg_num );
            score_parse_args( arg_num, -1, (const char**)arg_list );
          } Catch_anonymous {
            for( j=0; j<arg_num; j++ ) {
              free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
            }
            free_safe( arg_list, (sizeof( char* ) * arg_num) );
            printf( "score Throw T\n" );
            Throw 0;
          }
          for( j=0; j<arg_num; j++ ) {
            free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
          }
          free_safe( arg_list, (sizeof( char* ) * arg_num) );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Cannot find argument file %s specified with -f option", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          printf( "score Throw U\n" );
          Throw 0;
        }
      } else {
        printf( "score Throw V\n" );
        Throw 0;
      }

    } else if( strncmp( "-ec", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_assign = 1;
      score_add_arg( argv[i] );

    } else if( strncmp( "-ea", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_always = 1;
      score_add_arg( argv[i] );

    } else if( strncmp( "-ei", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_init = 1;
      score_add_arg( argv[i] );

    } else if( strncmp( "-ef", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_final = 1;
      score_add_arg( argv[i] );

    } else if( strncmp( "-ep", argv[i], 3 ) == 0 ) {

      info_suppl.part.excl_pragma = 1;
      score_add_arg( argv[i] );
      if( ((i+1) < argc) && (argv[i+1][0] != '-') ) {
        i++;
        pragma_coverage_name = strdup_safe( argv[i] );
        score_add_arg( argv[i] );
      } else {
        pragma_coverage_name = strdup_safe( "coverage" );
      }

    } else if( strncmp( "-e", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        search_add_no_score_funit( argv[i] );
        score_add_arg( argv[i-1] );
        score_add_arg( argv[i] );
      } else {
        printf( "score Throw W\n" );
        Throw 0;
      }

    } else if( strncmp( "-vcd", argv[i], 4 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        switch( dump_mode ) {
          case DUMP_FMT_NONE :
            if( file_exists( argv[i] ) ) {
              dump_file = strdup_safe( argv[i] );
              dump_mode = DUMP_FMT_VCD;
              score_add_arg( argv[i-1] );
              score_add_arg( argv[i] );
            } else {
              unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "VCD dumpfile not found \"%s\"", argv[i] );
              assert( rv < USER_MSG_LENGTH );
              print_output( user_msg, FATAL, __FILE__, __LINE__ );
              printf( "score Throw X\n" );
              Throw 0;
            }
            break;
          case DUMP_FMT_VCD :
            print_output( "Only one -vcd option is allowed on the score command-line", FATAL, __FILE__, __LINE__ );
            printf( "score Throw Y\n" );
            Throw 0;
            /*@-unreachable@*/
            break;
            /*@=unreachable@*/
          case DUMP_FMT_LXT :
            print_output( "Both the -vcd and -lxt options were specified on the command-line", FATAL, __FILE__, __LINE__ );
            printf( "score Throw Z\n" );
            Throw 0;
            /*@-unreachable@*/
            break;
            /*@=unreachable@*/
          default :
            assert( 0 );
            break;
        }
      } else {
        printf( "score Throw AA\n" );
        Throw 0;
      }

    } else if( strncmp( "-lxt", argv[i], 4 ) == 0 ) {
 
      if( check_option_value( argc, argv, i ) ) {
        i++; 
        switch( dump_mode ) {
          case DUMP_FMT_NONE :
            if( file_exists( argv[i] ) ) {
              dump_file = strdup_safe( argv[i] );
              dump_mode = DUMP_FMT_LXT;
              score_add_arg( argv[i-1] );
              score_add_arg( argv[i] );
            } else {
              unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "LXT dumpfile not found \"%s\"", argv[i] );
              assert( rv < USER_MSG_LENGTH );
              print_output( user_msg, FATAL, __FILE__, __LINE__ );
              printf( "score Throw AB\n" );
              Throw 0;
            }
            break;
          case DUMP_FMT_VCD :
            print_output( "Both the -vcd and -lxt options were specified on the command-line", FATAL, __FILE__, __LINE__ );
            printf( "score Throw AC\n" );
            Throw 0;
            /*@-unreachable@*/
            break;
            /*@=unreachable@*/
          case DUMP_FMT_LXT :
            print_output( "Only one -lxt option is allowed on the score command-line", FATAL, __FILE__, __LINE__ );
            printf( "score Throw AD\n" );
            Throw 0;
            /*@-unreachable@*/
            break;
            /*@=unreachable@*/
          default :
            assert( 0 );
            break;
        }
      } else {
        printf( "score Throw AE\n" );
        Throw 0;
      }

    } else if( strncmp( "-vpi_ts", argv[i], 7 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( vpi_timescale != NULL ) {
          print_output( "Only one -vpi_ts option is allowed on the score command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
          if( (i == argc) || (argv[i][0] == '-') ) {
            i--;
          }
        } else {
          process_timescale( argv[i], FALSE );
          vpi_timescale = strdup_safe( argv[i] );
          score_add_arg( argv[i-1] );
          score_add_arg( argv[i] );
        }
      } else {
        printf( "score Throw AF\n" );
        Throw 0;
      }

    } else if( strncmp( "-vpi", argv[i], 4 ) == 0 ) {

      i++;
      if( vpi_file != NULL ) {
        print_output( "Only one -vpi option is allowed on the score command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        if( (i == argc) || (argv[i][0] == '-') ) {
          i--;
        }
      } else {
        if( (i < argc) && (argv[i][0] != '-') ) {
          vpi_file = strdup_safe( argv[i] );
          score_add_arg( argv[i-1] );
          score_add_arg( argv[i] );
        } else {
          vpi_file = strdup_safe( DFLT_VPI_NAME );
          i--;
          score_add_arg( argv[i] );
        }
      }

    } else if( strncmp( "-v", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        search_add_file( argv[i] );
        score_add_arg( argv[i-1] );
        score_add_arg( argv[i] );
      } else {
        printf( "score Throw AG\n" );
        Throw 0;
      }

    } else if( strncmp( "+libext+", argv[i], 8 ) == 0 ) {

      search_add_extensions( argv[i] + 8 );
      score_add_arg( argv[i] );

    } else if( strncmp( "-D", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        score_parse_define( argv[i] );
        score_add_arg( argv[i-1] );
        score_add_arg( argv[i] );
      } else {
        printf( "score Throw AH\n" );
        Throw 0;
      }
 
    } else if( strncmp( "-p", argv[i], 2 ) == 0 ) {
      
      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( ppfilename != NULL ) {
          print_output( "Only one -p option is allowed on the score command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( is_variable( argv[i] ) ) {
            ppfilename = strdup_safe( argv[i] );
            score_add_arg( argv[i-1] );
            score_add_arg( argv[i] );
          } else {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unrecognizable filename %s specified for -p option.", argv[i] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "score Throw AI\n" );
            Throw 0;
          }
        }
      } else {
        printf( "score Throw AJ\n" );
        Throw 0;
      }
        
    } else if( strncmp( "-P", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        char* tmp = strdup_safe( argv[i+1] );
        Try {
          i++;
          ptr = tmp;
          while( (*ptr != '\0') && (*ptr != '=') ) {
            ptr++;
          }
          if( *ptr == '\0' ) {
            print_output( "Option -P must specify a value to assign.  See \"covered score -h\" for more information.",
                          FATAL, __FILE__, __LINE__ );
            printf( "score Throw AK\n" );
            Throw 0;
          } else {
            score_add_arg( argv[i-1] );
            score_add_arg( argv[i] );
            *ptr = '\0';
            ptr++;
            defparam_add( tmp, vector_from_string( &ptr, FALSE ) );
          }
        } Catch_anonymous {
          free_safe( tmp, (strlen( tmp ) + 1) );
          printf( "score Throw AL\n" );
          Throw 0;
        }
        free_safe( tmp, (strlen( tmp ) + 1) );
      } else {
        printf( "score Throw AM\n" );
        Throw 0;
      }
      
    } else if( strncmp( "-T", argv[i], 2 ) == 0 ) {
      
      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( delay_expr_type != DELAY_EXPR_DEFAULT ) {
          print_output( "Only one -T option is allowed on the score command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( strcmp( argv[i], "min" ) == 0 ) {
            delay_expr_type = DELAY_EXPR_MIN;
            score_add_arg( argv[i-1] );
            score_add_arg( argv[i] );
          } else if( strcmp( argv[i], "max" ) == 0 ) {
            delay_expr_type = DELAY_EXPR_MAX;
            score_add_arg( argv[i-1] );
            score_add_arg( argv[i] );
          } else if( strcmp( argv[i], "typ" ) == 0 ) {
            delay_expr_type = DELAY_EXPR_TYP;
            score_add_arg( argv[i-1] );
            score_add_arg( argv[i] );
          } else {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown -T value (%s).  Please specify min, max or typ.", argv[i] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "score Throw AN\n" );
            Throw 0;
          }
        }
      } else {
        printf( "score Throw AO\n" );
        Throw 0;
      }

    } else if( strncmp( "-r", argv[i], 2 ) == 0 ) {

      switch( argv[i][2] ) {
        case 'E'  :  flag_race_check  = FATAL;    break;
        case 'W'  :  flag_race_check  = WARNING;  break;
        case 'I'  :  flag_check_races = FALSE;
        case 'S'  :
        case '\0' :  flag_race_check  = NORMAL;   break;
        case 'P'  :
          if( argv[i][3] == '=' ) {
            pragma_racecheck_name = strdup_safe( argv[i] + 4 );
          } else {
            pragma_racecheck_name = strdup_safe( "racecheck" );
          }
          break;
        default   :
          {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown race condition value %c (available types are E, W, S, I or P)", argv[i][2] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "score Throw AP\n" );
            Throw 0;
          }
          /*@-unreachable@*/
          break;
          /*@=unreachable@*/
      }
      score_add_arg( argv[i] );

    } else if( strncmp( "-S", argv[i], 2 ) == 0 ) {

      flag_display_sim_stats = TRUE;
      score_add_arg( argv[i] );

    } else if( strncmp( "-A", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( strncmp( argv[i], "ovl", 3 ) == 0 ) {
          info_suppl.part.assert_ovl = 1;
          define_macro( "OVL_VERILOG",  "1" );
          define_macro( "OVL_COVER_ON", "1" );
          score_add_arg( argv[i-1] );
          score_add_arg( argv[i] );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown -A value (%s).  Please specify ovl.", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          printf( "score Throw AQ\n" );
          Throw 0;
        }
      } else {
        printf( "score Throw AR\n" );
        Throw 0;
      }

    } else if( strncmp( "-g", argv[i], 2 ) == 0 ) {

      int  generation;
      char tmp[256];

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( argv[i][(strlen( argv[i] ) - 1)] == '1' ) {
          generation = GENERATION_1995;
        } else if( argv[i][(strlen( argv[i] ) - 1)] == '2' ) {
          generation = GENERATION_2001;
        } else if( argv[i][(strlen( argv[i] ) - 1)] == '3' ) {
          generation = GENERATION_SV;
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown generation value '%c'.  Legal values are 1, 2 or 3.\n", argv[i][(strlen( argv[i] ) - 1)] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ ); 
          printf( "score Throw AS\n" );
          Throw 0;
        }
        if( strlen( argv[i] ) == 1 ) {
          flag_global_generation = generation;
        } else {
          strcpy( tmp, argv[i] );
          if( tmp[(strlen( tmp ) - 2)] == '=' ) {
            str_link* strl;
            tmp[(strlen( tmp ) - 2)] = '\0';
            strl        = str_link_add( strdup_safe( tmp ), &gen_mod_head, &gen_mod_tail );
            strl->suppl = generation;
          } else {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal -g syntax \"%s\".  See \"covered score -h\" for correct syntax.", tmp );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "score Throw AT\n" );
            Throw 0;
          }
        }
        score_add_arg( argv[i-1] );
        score_add_arg( argv[i] );
      } else {
        printf( "score Throw AU\n" );
        Throw 0;
      }

    } else if( strncmp( "-cli", argv[i], 2 ) == 0 ) {

#ifdef DEBUG_MODE
      i++;
      if( flag_use_command_line_debug ) {
        print_output( "Only one -cli option is allowed on the score command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        if( (i == argc) || (argv[i][0] == '-') ) {
          i--;
        }
      } else {
        if( (i < argc) && (argv[i][0] != '-') ) {
          cli_read_hist_file( argv[i] );
          score_add_arg( argv[i-1] );
          score_add_arg( argv[i] );
        } else {
          i--;
          score_add_arg( argv[i] );
        }
        flag_use_command_line_debug = TRUE;
      }
#else
      print_output( "Command-line debugger (-cli option) is not available because Covered was not configured with the --enable-debug option", FATAL, __FILE__, __LINE__ );
      printf( "score Throw AV\n" );
      Throw 0;
#endif

    } else {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown score command option \"%s\".  See \"covered score -h\" for more information.", argv[i] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      printf( "score Throw AW\n" );
      Throw 0;

    }

    i++;

  }

  /* If the -A option was not specified, add all OVL modules to list of no-score modules */
  ovl_add_assertions_to_no_score_list( info_suppl.part.assert_ovl );
    
  /* Get the current directory */
  rv = getcwd( score_run_path, 4096 );
  assert( rv != NULL );

}

/*!
 \param argc      Number of arguments in score command-line.
 \param last_arg  Index of last parsed argument in list.
 \param argv      Arguments from command-line to parse.

 Performs score command functionality.
*/
void command_score( int argc, int last_arg, const char** argv ) { PROFILE(COMMAND_SCORE);

  unsigned int rv;  /* Return value from snprintf calls */

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, NORMAL, __FILE__, __LINE__ );

  Try {

    /* Parse score command-line */
    score_parse_args( argc, last_arg, argv );

    if( output_db == NULL ) {
      output_db = strdup_safe( DFLT_OUTPUT_CDD );
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

      rv = snprintf( user_msg, USER_MSG_LENGTH, "Outputting VPI file %s...", vpi_file );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      score_generate_top_vpi_module( vpi_file, output_db, top_instance );
      score_generate_pli_tab_file( vpi_file, top_module );

    /* Read dumpfile and score design */
    } else if( dump_mode != DUMP_FMT_NONE ) {

      switch( dump_mode ) {
        case DUMP_FMT_VCD :  rv = snprintf( user_msg, USER_MSG_LENGTH, "Scoring VCD dumpfile %s...", dump_file );  break;
        case DUMP_FMT_LXT :  rv = snprintf( user_msg, USER_MSG_LENGTH, "Scoring LXT dumpfile %s...", dump_file );  break;
      }
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      parse_and_score_dumpfile( output_db, dump_file, dump_mode );
      print_output( "", NORMAL, __FILE__, __LINE__ );

    }

    if( dump_mode != DUMP_FMT_NONE ) {
      print_output( "***  Scoring completed successfully!  ***\n", NORMAL, __FILE__, __LINE__ );
    }
    /*@-duplicatequals -formattype@*/
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Dynamic memory allocated:   %llu bytes", largest_malloc_size );
    assert( rv < USER_MSG_LENGTH );
    /*@=duplicatequals =formattype@*/
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    print_output( "", NORMAL, __FILE__, __LINE__ );

    /* Display simulation statistics if specified */
    if( flag_display_sim_stats ) {
      perf_output_inst_report( stdout );
    }

  } Catch_anonymous {}

  /* Close database */
  db_close();

  /* Deallocate memory for search engine */
  search_free_lists();

  /* Deallocate memory for defparams */
  defparam_dealloc();

  free_safe( output_db, (strlen( output_db ) + 1) );
  free_safe( dump_file, (strlen( dump_file ) + 1) );
  free_safe( vpi_file, (strlen( vpi_file ) + 1) );
  free_safe( top_module, (strlen( top_module ) + 1) );
  free_safe( ppfilename, (strlen( ppfilename ) + 1) );
  ppfilename = NULL;

  free_safe( directive_filename, (strlen( directive_filename ) + 1) );
  free_safe( top_instance, (strlen( top_instance ) + 1) );
  free_safe( vpi_timescale, (strlen( vpi_timescale ) + 1) );

  PROFILE_END;

}

/*
 $Log$
 Revision 1.118  2008/03/17 05:26:17  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.117  2008/03/14 22:00:20  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.116  2008/03/11 22:06:49  phase1geo
 Finishing first round of exception handling code.

 Revision 1.115  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.114  2008/02/22 20:39:22  phase1geo
 More updates for exception handling.

 Revision 1.113  2008/02/10 03:33:13  phase1geo
 More exception handling added and fixed remaining splint errors.

 Revision 1.112  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.111  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.110  2008/02/01 07:03:20  phase1geo
 Fixing bugs in pragma exclusion code.  Added diagnostics to regression suite
 to verify that we correctly exclude/include signals when pragmas are set
 around a register instantiation and the -ep is present/not present, respectively.
 Full regression passes at this point.  Fixed bug in vsignal.c where the excluded
 bit was getting lost when a CDD file was read back in.  Also fixed bug in toggle
 coverage reporting where a 1 -> 0 bit transition was not getting excluded when
 the excluded bit was set for a signal.

 Revision 1.109  2008/02/01 06:37:08  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.108  2008/01/21 21:39:55  phase1geo
 Bug fix for bug 1876376.

 Revision 1.107  2008/01/17 06:03:08  phase1geo
 Completing regression runs.

 Revision 1.106  2008/01/16 23:10:33  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.105  2008/01/15 23:01:15  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.104  2008/01/14 05:08:45  phase1geo
 Fixing bug created while doing splint updates.

 Revision 1.103  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.102  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.101  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.100  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.99  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.98  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.97  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.96  2007/08/07 02:23:32  phase1geo
 Fixing bug 1687409.

 Revision 1.95  2007/04/12 03:46:30  phase1geo
 Fixing bugs with CLI.  History and history file saving/loading is implemented
 and working as desired.

 Revision 1.94  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.93  2007/04/11 03:04:13  phase1geo
 Fixing bug 1688487.

 Revision 1.92  2007/03/13 22:12:59  phase1geo
 Merging changes to covered-0_5-branch to fix bug 1678931.

 Revision 1.91.2.1  2007/03/13 22:05:10  phase1geo
 Fixing bug 1678931.  Updated regression.

 Revision 1.91  2006/12/01 19:05:11  phase1geo
 Fixing the usage of -vpi_ts option.  Updating its -h usage information as well.

 Revision 1.90  2006/11/26 05:23:38  phase1geo
 Adding several more timescale diagnostics to regression suite.  Still have a couple
 of failing diagnostics in normal regression runs.  Checkpointing.

 Revision 1.89  2006/11/25 04:24:40  phase1geo
 Adding initial code to fully support the timescale directive and its usage.
 Added -vpi_ts score option to allow the user to specify a top-level timescale
 value for the generated VPI file (this code has not been tested at this point,
 however).  Also updated diagnostic Makefile to get the VPI shared object files
 from the current lib directory (instead of the checked in one).

 Revision 1.88  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.87  2006/11/17 23:17:12  phase1geo
 Fixing bug in score command where parameter override values were not being saved
 off properly in the CDD file.  Also fixing bug when a parameter is found in a VCD
 file (ignoring its usage).  Updated regressions for these changes.

 Revision 1.86  2006/10/13 15:56:02  phase1geo
 Updating rest of source files for compiler warnings.

 Revision 1.85  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.84  2006/08/31 04:02:02  phase1geo
 Adding parsing support for assertions and properties.  Adding feature to
 highlighting support that looks up the generation for the given module and
 highlights accordingly.

 Revision 1.83  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.82  2006/08/18 04:41:14  phase1geo
 Incorporating bug fixes 1538920 and 1541944.  Updated regressions.  Only
 event1.1 does not currently pass (this does not pass in the stable version
 yet either).

 Revision 1.81  2006/08/11 22:36:01  phase1geo
 Fixing bug 1538922.

 Revision 1.80  2006/08/10 22:35:14  phase1geo
 Updating with fixes for upcoming 0.4.7 stable release.  Updated regressions
 for this change.  Full regression still fails due to an unrelated issue.

 Revision 1.79  2006/08/06 05:02:59  phase1geo
 Documenting and adding warning message to parse.c for the -rI option.

 Revision 1.78  2006/08/06 04:36:20  phase1geo
 Fixing bugs 1533896 and 1533827.  Also added -rI option that will ignore
 the race condition check altogether (has not been verified to this point, however).

 Revision 1.77  2006/07/14 18:53:32  phase1geo
 Fixing -g option for keywords.  This seems to be working and I believe that
 regressions are passing here as well.

 Revision 1.76  2006/07/13 22:24:57  phase1geo
 We are really broke at this time; however, more code has been added to support
 the -g score option.

 Revision 1.75  2006/07/13 05:31:52  phase1geo
 Adding -g option to score command parser/usage information.  Still a lot of
 work to go before this feature is complete.

 Revision 1.74  2006/05/03 22:49:42  phase1geo
 Causing all files to be preprocessed when written to the file viewer.  I'm sure that
 I am breaking all kinds of things at this point, but things do work properly on a few
 select test cases so I'm checkpointing here.

 Revision 1.73  2006/05/02 21:49:41  phase1geo
 Updating regression files -- all but three diagnostics pass (due to known problems).
 Added SCORE_ARGS line type to CDD format which stores the directory that the score
 command was executed from as well as the command-line arguments to the score
 command.

 Revision 1.72  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.71  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.70  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

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

