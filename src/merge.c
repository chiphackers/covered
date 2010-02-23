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
 \file     merge.c
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
#include <stdlib.h>

#include "binding.h"
#include "db.h"
#include "defines.h"
#include "info.h"
#include "merge.h"
#include "sim.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern int          merged_code;
extern char         user_msg[USER_MSG_LENGTH];
extern char*        cdd_message;


/*!
 Specifies the output filename of the CDD file that contains the merged data.
*/
char* merged_file = NULL;

/*!
 Pointer to head of list containing names of the input CDD files.
*/
str_link* merge_in_head = NULL;

/*!
 Pointer to tail of list containing names of the input CDD files.
*/
str_link* merge_in_tail = NULL;

/*!
 Pointer to last input name from command-line.
*/
str_link* merge_in_cl_last = NULL;

/*!
 Specifies the number of merged CDD files.
*/
int merge_in_num  = 0;

/*!
 Specifies the value of the -er option.
*/
int merge_er_value = MERGE_ER_NONE;


/*!
 Outputs usage informaiton to standard output for merge command.
*/
static void merge_usage() {

  printf( "\n" );
  printf( "Usage:  covered merge (-h | [<options>] <existing_database> <database_to_merge>+)\n" );
  printf( "\n" );
  printf( "   -h                         Displays this help information.\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -o <filename>           File to output new database to.  If this argument is not\n" );
  printf( "                                specified, the <existing_database> is used as the output\n" );
  printf( "                                database name.\n" );
  printf( "      -f <filename>           Name of file containing additional arguments to parse.\n" );
  printf( "      -d <directory>          Directory to search for CDD files to include.  This option is used in\n" );
  printf( "                                conjunction with the -ext option which specifies the file extension\n" );
  printf( "                                to use for determining which files in the directory are CDD files.\n" );
  printf( "      -er <value>             Specifies how to handle exclusion reason resolution.  If two or more CDD files being\n" );
  printf( "                                merged have exclusion reasons specified for the same coverage point, the exclusion\n" );
  printf( "                                reason needs to be resolved (unless it is the same string value).  If this option\n" );
  printf( "                                is not specified and a conflict is found, Covered will interactively request input\n" );
  printf( "                                for each exclusion as to how to handle it.  If this option is specified, it tells\n" );
  printf( "                                Covered how to handle all exclusion reason conflicts.  The values are as follows:\n" );
  printf( "                                  first - CDD file that contained the first exclusion reason is used.\n" );
  printf( "                                  last  - CDD file that contained the last exclusion reason is used.\n" );
  printf( "                                  all   - All exclusion reasons are used (concatenated).\n" );
  printf( "                                  new   - Use the newest exclusion reason specified.\n" );
  printf( "                                  old   - Use the oldest exclusion reason specified.\n" );
  printf( "      -ext <extension>        Used in conjunction with the -d option.  If no -ext options are specified\n" );
  printf( "                                on the command-line, the default value of '.cdd' is used.  Note that\n" );
  printf( "                                a period (.) should be specified.\n" );
  printf( "      -m <message>            Allows the user to specify information about this CDD file.  This information\n" );
  printf( "                                can be anything (messages with whitespace should be surrounded by double-quotation\n" );
  printf( "                                marks), but may include something about the simulation arguments to more easily\n" );
  printf( "                                link the CDD file to its simulation for purposes of recreating the CDD file.\n" );
  printf( "\n" );

}

/*!
 \return Returns TRUE if the help option was parsed.

 \throws anonymous Throw Throw Throw

 Parses the merge argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the merge operation, an error message is displayed to the
 user.
*/
static bool merge_parse_args(
  int          argc,      /*!< Number of arguments in argument list argv */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< Argument list passed to this program */
) {

  int       i;
  str_link* strl;
  str_link* ext_head   = NULL;
  str_link* ext_tail   = NULL;
  str_link* dir_head   = NULL;
  str_link* dir_tail   = NULL;
  bool      help_found = FALSE;

  i = last_arg + 1;

  while( (i < argc) && !help_found ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      merge_usage();
      help_found = TRUE;

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {
    
      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( is_legal_filename( argv[i] ) ) {
          merged_file = strdup_safe( argv[i] );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output file \"%s\" is not writable", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-f", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        char**       arg_list = NULL;
        int          arg_num  = 0;
        unsigned int j;
        i++;
        Try {
          read_command_file( argv[i], &arg_list, &arg_num );
          help_found = merge_parse_args( arg_num, -1, (const char**)arg_list );
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

    } else if( strncmp( "-d", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( directory_exists( argv[i] ) ) {
          (void)str_link_add( strdup_safe( argv[i] ), &dir_head, &dir_tail );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Specified -d directory (%s) does not exist", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-er", argv[i], 3 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( strncmp( "first", argv[i], 5 ) == 0 ) {
          merge_er_value = MERGE_ER_FIRST;
        } else if( strncmp( "last", argv[i], 4 ) == 0 ) {
          merge_er_value = MERGE_ER_LAST;
        } else if( strncmp( "all", argv[i], 3 ) == 0 ) {
          merge_er_value = MERGE_ER_ALL;
        } else if( strncmp( "new", argv[i], 3 ) == 0 ) {
          merge_er_value = MERGE_ER_NEW;
        } else if( strncmp( "old", argv[i], 3 ) == 0 ) {
          merge_er_value = MERGE_ER_OLD;
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal value to use for the -er option (%s).  Valid values are: first, last, all, new, old", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-ext", argv[i], 4 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        (void)str_link_add( strdup_safe( argv[i] ), &ext_head, &ext_tail );
      } else {
        Throw 0;
      } 

    } else if( strncmp( "-m", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cdd_message != NULL ) {
          print_output( "Only one -m option is allowed on the merge command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          cdd_message = strdup_safe( argv[i] );
        }
      } else {
        Throw 0;
      }

    } else {

      /* The name of a file to merge */
      if( file_exists( argv[i] ) ) {

        /* Create absolute filename */
        char* file = get_absolute_path( argv[i] );

        /* If we have not specified a merge file explicitly, set it implicitly to the first CDD file found */
        if( (merge_in_head == NULL) && (merged_file == NULL) ) {
          merged_file = strdup_safe( file );
        }

        /* Add the specified merge file to the list */
        (void)str_link_add( file, &merge_in_head, &merge_in_tail );

      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "CDD file (%s) does not exist", argv[i] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    }

    i++;

  }

  if( !help_found ) {

    Try {

      /* Load any merge files found in specified directories */
      strl = dir_head;
      while( strl != NULL ) {
        directory_load( strl->str, ext_head, &merge_in_head, &merge_in_tail );
        strl = strl->next;
      }

    } Catch_anonymous {
      str_link_delete_list( ext_head );
      str_link_delete_list( dir_head );
      Throw 0;
    }

    /* Set the last command-line pointer to the current tail */
    merge_in_cl_last = merge_in_tail;
  }

  /* Deallocate the temporary lists */
  str_link_delete_list( ext_head );
  str_link_delete_list( dir_head );

  return( help_found );
}

/*!
 Check number of merged files.
*/
void merge_check() { PROFILE(MERGE_CHECK);

  str_link* strl;

  /* Make sure that we have at least 2 files to merge */
  strl = merge_in_head;
  while( strl != NULL ) {
    merge_in_num++;
    strl = strl->next;
  }

  /* Check to make sure that the user specified at least two files to merge */
  if( merge_in_num < 2 ) {
    print_output( "Must specify at least two CDD files to merge", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /*
   If no -o option was specified and no merge files were specified, don't presume that the first file found in
   the directory will be that file.
  */
  if( merged_file == NULL ) {
    print_output( "Must specify the -o option or a specific CDD file for containing the merged results", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  PROFILE_END;

}

/*!
 Performs merge command functionality.
*/
void command_merge(
  int          argc,      /*!< Number of arguments in command-line to parse */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< List of arguments from command-line to parse */
) { PROFILE(COMMAND_MERGE);

  int          i;     /* Loop iterator */
  unsigned int rv;    /* Return value from snprintf calls */
  bool         error = FALSE;

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, HEADER, __FILE__, __LINE__ );

  Try {

    str_link* strl;
    bool      stop_merging;
    int       curr_leading_hier_num = 0;

    /* Parse score command-line */
    if( !merge_parse_args( argc, last_arg, argv ) ) {

      /* Check if merge could be executed */
      merge_check();

      /* Read in base database */
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Reading CDD file \"%s\"", merge_in_head->str );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      if( !db_read( merge_in_head->str, READ_MODE_MERGE_NO_MERGE ) ) {

        /* The read in CDD was empty so mark it as such */
        merge_in_head->suppl = 2;

      } else {

        /* If the currently read CDD didn't contain any merged CDDs it is a leaf CDD so mark it as such */
        if( (db_list[curr_db]->leading_hier_num - curr_leading_hier_num) == 1 ) {
          merge_in_head->suppl = 1;
        }
        curr_leading_hier_num = db_list[curr_db]->leading_hier_num;

      }

      /* Read in databases to merge */
      strl         = merge_in_head->next;
      stop_merging = (strl == merge_in_head);
      while( (strl != NULL) && !stop_merging ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Merging CDD file \"%s\"", strl->str );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, NORMAL, __FILE__, __LINE__ );
        if( !db_read( strl->str, READ_MODE_MERGE_NO_MERGE ) ) {

          /* The read in CDD was empty so mark it as such */
          merge_in_head->suppl = 2;

        } else {

          /* If we have not merged any CDD files from this CDD, this is a leaf CDD so mark it as such */
          if( (db_list[curr_db]->leading_hier_num - curr_leading_hier_num) == 1 ) {
            strl->suppl = 1;
          }
          curr_leading_hier_num = db_list[curr_db]->leading_hier_num;

        }

        stop_merging = (strl == merge_in_cl_last);
        strl         = strl->next;
      }

      /* Perform the tree merges */
      db_merge_instance_trees();

      /* Bind */
      bind_perform( TRUE, 0 );

      /* Write out new database to output file */
      db_write( merged_file, FALSE, TRUE );

      print_output( "\n***  Merging completed successfully!  ***", NORMAL, __FILE__, __LINE__ );

    }

  } Catch_anonymous {
    error = TRUE;
  }

  /* Close database */
  db_close();

  /* Deallocate other memory */
  str_link_delete_list( merge_in_head );
  free_safe( merged_file, (strlen( merged_file ) + 1) );

  if( error ) {
    Throw 0;
  }

  PROFILE_END;

}

