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


/*!
 Displays usage information for score command.
*/
void score_usage() {

  printf( "\n" );
  printf( "Usage:  covered score -t <top-level_module_name> -vcd <dumpfile> [<options>]\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -i <instance_name>      Instance name of top-level module.  Necessary if module\n" );
  printf( "                              to verify coverage is not the top-level module in the design.\n" );
  printf( "                              If not specified, -t value is used.\n" );
  printf( "      -o <database_filename>  Name of database to write coverage information to.\n" );
  printf( "      -I <directory>          Directory to find included Verilog files\n" );
  printf( "      -f <filename>           Name of file containing additional arguments to parse\n" );
  printf( "      -y <directory>          Directory to find unspecified Verilog files\n" );
  printf( "      -v <filename>           Name of specific Verilog file to score\n" );
  printf( "      -e <module_name>        Name of module to not score\n" );
  printf( "      -q                      Suppresses output to standard output\n" );
  printf( "      -h                      Displays this help information\n" );
  printf( "\n" );
  printf( "      +libext+.<extension>(+.<extension>)+\n" );
  printf( "                              Extensions of Verilog files to allow in scoring\n" );
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
bool read_command_file( char* cmd_file, char** arg_list, int* arg_num ) {

  bool      retval = TRUE;   /* Return value for this function       */
  str_link* head   = NULL;   /* Pointer to head element of arg list  */
  str_link* tail   = NULL;   /* Pointer to tail element of arg list  */
  FILE*     cmd_handle;      /* Pointer to command file              */
  char      tmp_str[1024];   /* Temporary holder for read argument   */
  char*     arg;             /* Temporary holder of current argument */
  str_link* curr;            /* Pointer to current str_link element  */

  if( file_exists( cmd_file ) ) {

    if( (cmd_handle = fopen( cmd_file, "r" )) != NULL ) {

      while( fscanf( cmd_handle, "%s", tmp_str ) == 1 ) {
        str_link_add( strdup( tmp_str ), &head, &tail );
      }

      fclose( cmd_handle );

      /* Create argument list */
      arg_list = (char**)malloc_safe( sizeof( char* ) * *arg_num );
      *arg_num = 0;

      curr = head;
      while( curr != NULL ) {
        arg_list[(*arg_num)] = strdup( curr->str );
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
 \param argc Number of arguments specified in argv parameter list.
 \param argv List of arguments to parse.
 \return Returns TRUE if successful in dealing with arguments; otherwise,
         returns FALSE.
*/
bool score_parse_args( int argc, char** argv ) {

  bool   retval  = TRUE;  /* Return value for this function  */
  int    i       = 2;     /* Loop iterator                   */
  char** arg_list;        /* List of command_line arguments  */
  int    arg_num = 0;     /* Number of arguments in arg_list */
  char   err_msg[4096];   /* Error message to display        */

  while( (i < argc) && retval ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      score_usage();
      retval = FALSE;

    } else if( strncmp( "-q", argv[i], 2 ) == 0 ) {

      set_output_suppression( TRUE );

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
        read_command_file( argv[i], arg_list, &arg_num );
        score_parse_args( arg_num, arg_list );
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
 \param argc  Number of arguments in score command-line.
 \param argv  Arguments from command-line to parse.
 \return Returns 0 if scoring is successful; otherwise, returns -1.

*/
int command_score( int argc, char** argv ) {

  int  retval = 0;   /* Return value for this function */
  char msg[4096];    /* Message to user                */

  /* Initialize error suppression value */
  set_output_suppression( FALSE );

  /* Parse score command-line */
  if( score_parse_args( argc, argv ) ) {

    printf( COVERED_HEADER );

    if( output_db == NULL ) {
      output_db = strdup( DFLT_OUTPUT_DB );
    }

    printf( "Scoring design...\n" );

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

    printf( "\n***  Scoring completed successfully!  ***\n" );
    printf( "\n" );
    printf( "Dynamic memory allocated:   %ld bytes\n", largest_malloc_size );
    snprintf( msg, 4096, "Allocated memory remaining: %ld bytes\n", curr_malloc_size );
    print_output( msg, NORMAL );
    printf( "\n" );

  }

  return( retval );

}

/* $Log$
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

