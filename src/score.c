/*!
 \file     score.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/29/2001
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "score.h"
#include "util.h"
#include "link.h"
#include "search.h"
#include "parse.h"


char* top_module   = NULL;    /*!< Name of top-level module to score              */
char* top_instance = NULL;    /*!< Name of top-level instance name                */
char* output_db    = NULL;    /*!< Name of output score database file to generate */
char* vcd_file     = NULL;    /*!< Name of VCD output file to parse               */

extern unsigned long largest_malloc_size;
extern unsigned long curr_malloc_size;


void define_macro( const char* name, const char* value );


/*!
 Displays usage information for score command.
*/
void score_usage() {

  printf( "\n" );
  printf( "Usage:  covered score -t <top-level_module_name> -vcd <dumpfile> [<options>]\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -i <instance_name>          Instance name of top-level module.  Necessary if module\n" );
  printf( "                                  to verify coverage is not the top-level module in the design.\n" );
  printf( "                                  If not specified, -t value is used.\n" );
  printf( "      -o <database_filename>      Name of database to write coverage information to.\n" );
  printf( "      -I <directory>              Directory to find included Verilog files\n" );
  printf( "      -f <filename>               Name of file containing additional arguments to parse\n" );
  printf( "      -y <directory>              Directory to find unspecified Verilog files\n" );
  printf( "      -v <filename>               Name of specific Verilog file to score\n" );
  printf( "      -e <module_name>            Name of module to not score\n" );
  printf( "      -D <define_name>(=<value>)  Defines the specified name to 1 or the specified value\n" );
  printf( "      -h                          Displays this help information\n" );
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

  bool      retval  = TRUE;  /* Return value for this function       */
  str_link* head    = NULL;  /* Pointer to head element of arg list  */
  str_link* tail    = NULL;  /* Pointer to tail element of arg list  */
  FILE*     cmd_handle;      /* Pointer to command file              */
  char      tmp_str[1024];   /* Temporary holder for read argument   */
  char*     arg;             /* Temporary holder of current argument */
  str_link* curr;            /* Pointer to current str_link element  */
  int       tmp_num = 0;     /* Temporary argument number holder     */

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

      snprintf( tmp_str, 1024, "Unable to open command file %s for reading", cmd_file );
      print_output( tmp_str, FATAL );
      retval = FALSE;

    }

  } else {

    snprintf( tmp_str, 1024, "Command file %s does not exist", cmd_file );
    print_output( tmp_str, FATAL );
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
  char   err_msg[4096];           /* Error message to display                      */
  int    j;                       /* Loop iterator 2                               */
  char*  ptr;                     /* Pointer to current character in defined value */

  while( (i < argc) && retval ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      score_usage();
      retval = FALSE;

    } else if( strncmp( "-i", argv[i], 2 ) == 0 ) {

      i++;
  
      if( is_variable( argv[i] ) ) {
        top_instance = strdup( argv[i] );
      } else {
        snprintf( err_msg, 4096, "Illegal top-level instance name specified \"%s\"", argv[i] );
        print_output( err_msg, FATAL );
        retval = FALSE;
      }

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      i++;
      if( is_directory( argv[i] ) ) {
        output_db = strdup( argv[i] );
      } else {
  	snprintf( err_msg, 4096, "Illegal output directory specified \"%s\"", argv[i] );
        print_output( err_msg, FATAL );
        retval = FALSE;
      }

    } else if( strncmp( "-t", argv[i], 2 ) == 0 ) {

      i++;
      if( is_variable( argv[i] ) ) {
        top_module = strdup( argv[i] );
      } else {
	snprintf( err_msg, 4096, "Illegal top-level module name specified \"%s\"", argv[i] );
        print_output( err_msg, FATAL );
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
          free_safe( arg_list[i] );
        }
        free_safe( arg_list );
      } else {
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
	snprintf( err_msg, 4096, "VCD dumpfile not found \"%s\"", argv[i] );
	print_output( err_msg, FATAL );
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

    } else {

      snprintf( err_msg, 4096, "Unknown score command option \"%s\".  See \"covered -h\" for more information.", argv[i] );
      print_output( err_msg, FATAL );
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

  int  retval = 0;   /* Return value for this function */
  char msg[4096];    /* Message to user                */

  /* Parse score command-line */
  if( score_parse_args( argc, last_arg, argv ) ) {

    snprintf( msg, 4096, COVERED_HEADER );
    print_output( msg, NORMAL );

    if( output_db == NULL ) {
      output_db = strdup( DFLT_OUTPUT_DB );
    }

    print_output( "Scoring design...", NORMAL );

    /* Initialize search engine */
    search_init();

    /* Parse design */
    parse_design( top_module, output_db );

    /* Read dumpfile and score design */
    parse_and_score_dumpfile( top_module, output_db, vcd_file );

    /* Deallocate memory for search engine */
    search_free_lists();

    free_safe( top_module );
    free_safe( output_db );
    free_safe( vcd_file );

    print_output( "\n***  Scoring completed successfully!  ***\n", NORMAL );
    snprintf( msg, 4096, "Dynamic memory allocated:   %ld bytes", largest_malloc_size );
    print_output( msg, NORMAL );
    snprintf( msg, 4096, "Allocated memory remaining: %ld bytes", curr_malloc_size );
    print_output( msg, DEBUG );
    print_output( "", NORMAL );

  }

  return( retval );

}

/* $Log$
/* Revision 1.13  2002/07/17 00:13:57  phase1geo
/* Added support for -e option and informally tested.
/*
/* Revision 1.12  2002/07/16 03:29:18  phase1geo
/* Updates to NEWS, INSTALL, ChangeLog for release.  Modifications to Verilog
/* diagnostic Makefile to make it easier to read.  Added user warning if -e
/* option is specified since this is not supported at this time.  Removed
/* mpatrol from configure.in.
/*
/* Revision 1.11  2002/07/11 15:10:00  phase1geo
/* Fixing -f option to score command.  This function was causing infinite loops
/* and massive memory consumption as a result of this.  Fixes bug 579946.
/*
/* Revision 1.10  2002/07/09 04:46:26  phase1geo
/* Adding -D and -Q options to covered for outputting debug information or
/* suppressing normal output entirely.  Updated generated documentation and
/* modified Verilog diagnostic Makefile to use these new options.
/*
/* Revision 1.9  2002/07/08 19:02:12  phase1geo
/* Adding -i option to properly handle modules specified for coverage that
/* are instantiated within a design without needing to parse parent modules.
/*
/* Revision 1.8  2002/07/08 16:06:33  phase1geo
/* Updating help information.
/*
/* Revision 1.7  2002/07/08 12:35:31  phase1geo
/* Added initial support for library searching.  Code seems to be broken at the
/* moment.
/*
/* Revision 1.6  2002/07/03 21:30:53  phase1geo
/* Fixed remaining issues with always statements.  Full regression is running
/* error free at this point.  Regenerated documentation.  Added EOR expression
/* operation to handle the or expression in event lists.
/*
/* Revision 1.5  2002/07/02 22:37:35  phase1geo
/* Changing on-line help command calling.  Regenerated documentation.
/* */

