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

#include "defines.h"
#include "score.h"
#include "util.h"
#include "link.h"
#include "search.h"
#include "parse.h"
#include "param.h"
#include "vector.h"


char* top_module      = NULL;                /*!< Name of top-level module to score                     */
char* top_instance    = NULL;                /*!< Name of top-level instance name                       */
char* output_db       = NULL;                /*!< Name of output score database file to generate        */
char* vcd_file        = NULL;                /*!< Name of VCD output file to parse                      */
int   delay_expr_type = DELAY_EXPR_DEFAULT;  /*!< Value to use when a delay expression with min:typ:max */

extern unsigned long largest_malloc_size;
extern unsigned long curr_malloc_size;
extern str_link*     use_files_head;
extern char          user_msg[USER_MSG_LENGTH];


void define_macro( const char* name, const char* value );


/*!
 Displays usage information for score command.
*/
void score_usage() {

  printf( "\n" );
  printf( "Usage:  covered score -t <top-level_module_name> [<options>]\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -vcd <dumpfile>              Name of dumpfile to score design with.  If this option\n" );
  printf( "                                   is not used, Covered will only create an initial CDD file\n" );
  printf( "                                   from the design and will not attempt to score the design.\n" );
  printf( "      -i <instance_name>           Verilog hierarchical scope of top-level module to score.\n" );
  printf( "                                   Necessary if module to verify coverage is not the top-level\n" );
  printf( "                                   module in the design.  If not specified, -t value is used.\n" );
  printf( "      -o <database_filename>       Name of database to write coverage information to.\n" );
  printf( "      -I <directory>               Directory to find included Verilog files.\n" );
  printf( "      -f <filename>                Name of file containing additional arguments to parse.\n" );
  printf( "      -y <directory>               Directory to find unspecified Verilog files.\n" );
  printf( "      -v <filename>                Name of specific Verilog file to score.\n" );
  printf( "      -e <module_name>             Name of module to not score.\n" );
  printf( "      -D <define_name>(=<value>)   Defines the specified name to 1 or the specified value.\n" );
  printf( "      -P <parameter_scope>=<value> Performs a defparam on the specified parameter with value.\n" );
  printf( "      -T min|typ|max               Specifies value to use in delay expressions of the form min:typ:max.\n" );
  printf( "      -h                           Displays this help information.\n" );
  printf( "\n" );
  printf( "      +libext+.<extension>(+.<extension>)+\n" );
  printf( "                                  Extensions of Verilog files to allow in scoring\n" );
  printf( "\n" );
  printf( "    Note:\n" );
  printf( "      The top-level module specifies the module to begin scoring.  All\n" );
  printf( "      modules beneath this module in the hierarchy will also be scored\n" );
  printf( "      unless these modules are explicitly stated to not be scored using\n" );
  printf( "      the -e flag.\n" );
  printf( "\n" );

}
/*!
 \param cmd_file Name of file to read commands from.
 \param arg_list List of arguments found in specified command file.
 \param arg_num  Number of arguments in arg_list array.
 \return Returns TRUE if read of command file was successful; otherwise,
         returns FALSE.
*/
bool read_command_file( char* cmd_file, char*** arg_list, int* arg_num ) {

  bool      retval  = TRUE;  /* Return value for this function      */
  str_link* head    = NULL;  /* Pointer to head element of arg list */
  str_link* tail    = NULL;  /* Pointer to tail element of arg list */
  FILE*     cmd_handle;      /* Pointer to command file             */
  char      tmp_str[1024];   /* Temporary holder for read argument  */
  str_link* curr;            /* Pointer to current str_link element */
  int       tmp_num = 0;     /* Temporary argument number holder    */

  if( file_exists( cmd_file ) ) {

    if( (cmd_handle = fopen( cmd_file, "r" )) != NULL ) {

      while( fscanf( cmd_handle, "%s", tmp_str ) == 1 ) {
        str_link_add( strdup( tmp_str ), &head, &tail );
        tmp_num++;
      }

      fclose( cmd_handle );

      /* Create argument list */
      *arg_list = (char**)malloc_safe( sizeof( char* ) * tmp_num );
      *arg_num  = tmp_num;
      tmp_num   = 0;

      curr = head;
      while( curr != NULL ) {
        (*arg_list)[tmp_num] = strdup( curr->str );
        tmp_num++;
        curr = curr->next;
      }

      /* Delete list */
      str_link_delete_list( head );

    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open command file %s for reading", cmd_file );
      print_output( user_msg, FATAL );
      retval = FALSE;

    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Command file %s does not exist", cmd_file );
    print_output( user_msg, FATAL );
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

  bool   retval  = TRUE;          /* Return value for this function                */
  int    i       = last_arg + 1;  /* Loop iterator                                 */
  char** arg_list;                /* List of command_line arguments                */
  int    arg_num;                 /* Number of arguments in arg_list               */
  int    j;                       /* Loop iterator 2                               */
  char*  ptr;                     /* Pointer to current character in defined value */

  while( (i < argc) && retval ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      score_usage();
      retval = FALSE;

    } else if( strncmp( "-i", argv[i], 2 ) == 0 ) {

      i++;
      top_instance = strdup( argv[i] );

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      i++;
      if( is_directory( argv[i] ) ) {
        output_db = strdup( argv[i] );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Illegal output directory specified \"%s\"", argv[i] );
        print_output( user_msg, FATAL );
        retval = FALSE;
      }

    } else if( strncmp( "-t", argv[i], 2 ) == 0 ) {

      i++;
      if( is_variable( argv[i] ) ) {
        top_module = strdup( argv[i] );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Illegal top-level module name specified \"%s\"", argv[i] );
        print_output( user_msg, FATAL );
        retval = FALSE;
      }

    } else if( strncmp( "-I", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_include_path( argv[i] );

    } else if( strncmp( "-y", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_directory_path( argv[i] );

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
        print_output( user_msg, FATAL );
        retval = FALSE;
      }

    } else if( strncmp( "-e", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_no_score_module( argv[i] );

    } else if( strncmp( "-vcd", argv[i], 4 ) == 0 ) {

      i++;
      if( file_exists( argv[i] ) ) {
        vcd_file = strdup( argv[i] );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "VCD dumpfile not found \"%s\"", argv[i] );
        print_output( user_msg, FATAL );
        retval = FALSE;
      }

    } else if( strncmp( "-v", argv[i], 2 ) == 0 ) {

      i++;
      retval = search_add_file( argv[i] );

    } else if( strncmp( "+libext+.", argv[i], 8 ) == 0 ) {

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

    } else if( strncmp( "-P", argv[i], 2 ) == 0 ) {

      i++;
      ptr = argv[i];
      while( (*ptr != '\0') && (*ptr != '=') ) {
        ptr++;
      }
      if( *ptr == '\0' ) {
        print_output( "Option -P must specify a value to assign.  See \"covered score -h\" for more information.", FATAL );
        exit( 1 );
      } else {
        *ptr = '\0';
        ptr++;
        defparam_add( argv[i], vector_from_string( ptr ) );
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
        print_output( user_msg, FATAL );
        exit( 1 );
      }
        
    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Unknown score command option \"%s\".  See \"covered score -h\" for more information.", argv[i] );
      print_output( user_msg, FATAL );
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
    print_output( user_msg, NORMAL );

    if( output_db == NULL ) {
      output_db = strdup( DFLT_OUTPUT_DB );
    }

    /* Parse design */
    if( use_files_head != NULL ) {
      print_output( "Reading design...", NORMAL );
      search_init();
      parse_design( top_module, output_db );
      print_output( "", NORMAL );
    }

    /* Read dumpfile and score design */
    if( vcd_file != NULL ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Scoring dumpfile %s...", vcd_file );
      print_output( user_msg, NORMAL );
      parse_and_score_dumpfile( output_db, vcd_file );
      print_output( "", NORMAL );
    }

    /* Deallocate memory for search engine */
    search_free_lists();

    free_safe( output_db );
    free_safe( vcd_file );

    print_output( "***  Scoring completed successfully!  ***\n", NORMAL );
    snprintf( user_msg, USER_MSG_LENGTH, "Dynamic memory allocated:   %ld bytes", largest_malloc_size );
    print_output( user_msg, NORMAL );
    snprintf( user_msg, USER_MSG_LENGTH, "Allocated memory remaining: %ld bytes", curr_malloc_size );
    print_output( user_msg, DEBUG );
    print_output( "", NORMAL );

  }

  return( retval );

}

/*
 $Log$
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

