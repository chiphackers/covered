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
 \file     rank.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     6/28/2008
*/

#include <stdio.h>
#include <assert.h>

#include "comb.h"
#include "defines.h"
#include "expr.h"
#include "fsm.h"
#include "func_iter.h"
#include "link.h"
#include "profiler.h"
#include "rank.h"
#include "util.h"
#include "vsignal.h"


extern char           user_msg[USER_MSG_LENGTH];
extern const exp_info exp_op_info[EXP_OP_NUM];
extern db**           db_list;
extern uint64         num_timesteps;
extern bool           quiet_mode;
extern bool           terse_mode;
extern bool           debug_mode;
extern int64          largest_malloc_size;
extern bool           report_line;
extern bool           report_toggle;
extern bool           report_combination;
extern bool           report_fsm;
extern bool           report_assertion;
extern bool           report_memory;
extern bool           allow_multi_expr;


/*!
 Pointer to head of list of CDD filenames that need to be read in.
*/
static str_link* rank_in_head = NULL;

/*!
 Pointer to tail of list of CDD filenames that need to be read in.
*/
static str_link* rank_in_tail = NULL;

/*!
 File to be used for outputting rank information.
*/
static char* rank_file = NULL;

/*!
 Array containing the number of coverage points for each metric for all compressed CDD coverage structures.
*/
static uint64 num_cps[CP_TYPE_NUM] = {0};

/*!
 Array containing the weights to be used for each of the CDD metric types.
*/
//static unsigned int cdd_type_weight[CP_TYPE_NUM] = {10,1,2,5,3,0};
static unsigned int cdd_type_weight[CP_TYPE_NUM] = {1,1,1,1,1,0};

/*!
 Set to TRUE when the user has specified the corresponding weight value on the command-line.  Allows us to
 display a warning message to the user when multiple values are specified.
*/
static bool cdd_type_set[CP_TYPE_NUM] = {0};

/*!
 If set to TRUE, outputs only the names of the CDD files in the order that they should be run.  This value
 is set to TRUE when the -names_only option is specified.
*/
static bool flag_names_only = FALSE;

/*!
 Specifies the number of CDDs that must hit each coverage point before a CDD will be considered
 unneeded.
*/
static unsigned int cp_depth = 0;

/*!
 Specifies the string length of the longest CDD name
*/
static unsigned int longest_name_len = 0;

/*!
 If set to TRUE, outputs behind the scenes output during the rank selection process.
*/
static bool rank_verbose = FALSE;


#ifdef OBSOLETE
/*!
 \return Returns the number of bits that are set in the given unsigned char
*/
static inline unsigned int rank_count_bits_uchar(
  unsigned char v   /*!< Character to count bits for */
) {

  v = (v & 0x55) + ((v >> 1) & 0x55);
  v = (v & 0x33) + ((v >> 2) & 0x33);
  v = (v & 0x0f) + ((v >> 4) & 0x0f);

  return( (unsigned int)v );
  
}

/*!
 \return Returns the number of bits that are set in the given 32-bit unsigned integer.
*/
static inline unsigned int rank_count_bits_uint32(
  uint32 v  /*!< 32-bit value to count bits for */
) {

  v = (v & 0x55555555) + ((v >>  1) & 0x55555555);
  v = (v & 0x33333333) + ((v >>  2) & 0x33333333);
  v = (v & 0x0f0f0f0f) + ((v >>  4) & 0x0f0f0f0f);
  v = (v & 0x00ff00ff) + ((v >>  8) & 0x00ff00ff);
  v = (v & 0x0000ffff) + ((v >> 16) & 0x0000ffff);

  return( (unsigned int)v );

}

/*!
 \return Returns the number of bits that are set in the given 64-bit unsigned integer.
*/
static inline unsigned int rank_count_bits_uint64(
  uint64 v  /*!< 64-bit value to count bits for */
) {

  v = (v & 0x5555555555555555LL) + ((v >>  1) & 0x5555555555555555LL);
  v = (v & 0x3333333333333333LL) + ((v >>  2) & 0x3333333333333333LL);
  v = (v & 0x0f0f0f0f0f0f0f0fLL) + ((v >>  4) & 0x0f0f0f0f0f0f0f0fLL);
  v = (v & 0x00ff00ff00ff00ffLL) + ((v >>  8) & 0x00ff00ff00ff00ffLL);
  v = (v & 0x0000ffff0000ffffLL) + ((v >> 16) & 0x0000ffff0000ffffLL);
  v = (v & 0x00000000ffffffffLL) + ((v >> 32) & 0x00000000ffffffffLL);
  
  return( (unsigned int)v );

}
#endif

/*!
 \return Returns a pointer to a newly allocated and initialized compressed CDD coverage structure.
*/
comp_cdd_cov* rank_create_comp_cdd_cov(
  const char* cdd_name,  /*!< Name of CDD file that this structure was created from */
  bool        required,  /*!< Set to TRUE if this CDD is required to be ranked by the user */
  uint64      timesteps  /*!< Number of simulation timesteps that occurred in the CDD */
) { PROFILE(RANK_CREATE_COMP_CDD_COV);

  comp_cdd_cov* comp_cov;
  unsigned int  i;

  /* Allocate and initialize */
  comp_cov             = (comp_cdd_cov*)malloc_safe( sizeof( comp_cdd_cov ) );
  comp_cov->cdd_name   = strdup_safe( cdd_name );
  comp_cov->timesteps  = timesteps;
  comp_cov->total_cps  = 0;
  comp_cov->unique_cps = 0;
  comp_cov->required   = required;

  /* Save longest name length */
  if( strlen( comp_cov->cdd_name ) > longest_name_len ) {
    longest_name_len = strlen( comp_cov->cdd_name );
  }

  for( i=0; i<CP_TYPE_NUM; i++ ) {
    comp_cov->cps_index[i] = 0;
    if( num_cps[i] > 0 ) {
      comp_cov->cps[i] = (ulong*)calloc_safe( (UL_DIV(num_cps[i]) + 1), sizeof( ulong ) );
    } else {
      comp_cov->cps[i] = NULL;
    }
  }

  PROFILE_END;

  return( comp_cov );

}

/*!
 Deallocates the specified compressed CDD coverage structure.
*/
void rank_dealloc_comp_cdd_cov(
  comp_cdd_cov* comp_cov  /*!< Pointer to compressed CDD coverage structure to deallocate */
) { PROFILE(RANK_DEALLOC_COMP_CDD_COV);

  if( comp_cov != NULL ) {

    unsigned int i;

    /* Deallocate name */
    free_safe( comp_cov->cdd_name, (strlen( comp_cov->cdd_name ) + 1) );

    /* Deallocate compressed coverage point information */
    for( i=0; i<CP_TYPE_NUM; i++ ) {
      free_safe( comp_cov->cps[i], (sizeof( ulong ) * (UL_DIV( num_cps[i] ) + 1)) );
    }

    /* Now deallocate ourselves */
    free_safe( comp_cov, sizeof( comp_cdd_cov ) );

  }

  PROFILE_END;

}

/*!
 Outputs usage information to standard output for rank command.
*/
static void rank_usage() {

  printf( "\n" );
  printf( "Usage:  covered rank (-h | ([<options>] <database_to_rank> <database_to_rank>+)\n" );
  printf( "\n" );
  printf( "   -h                           Displays this help information.\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -depth <number>           Specifies the minimum number of CDD files to hit each coverage point.\n" );
  printf( "                                  The value of <number> should be a value of 1 or more.  Default is 1.\n" );
  printf( "      -names-only               If specified, outputs only the needed CDD filenames that need to be\n" );
  printf( "                                  run in the order they need to be run.  If this option is not set, a\n" );
  printf( "                                  report-style output is provided with additional information.\n" );
  printf( "      -f <filename>             Name of file containing additional arguments to parse.\n" );
  printf( "      -required-list <filename> Name of file containing list of CDD files which are required to be in the\n" );
  printf( "                                  list of ranked CDDs to be run.\n" );
  printf( "      -required-cdd <filename>  Name of CDD file that is required to be in the list of ranked CDDs to be run.\n" );
  printf( "      -d <directory>            Directory to search for CDD files to include.  This option is used in\n" );
  printf( "                                  conjunction with the -ext option which specifies the file extension\n" );
  printf( "                                  to use for determining which files in the directory are CDD files.\n" );
  printf( "      -ext <extension>          Used in conjunction with the -d option.  If no -ext options are specified\n" );
  printf( "                                  on the command-line, the default value of '.cdd' is used.  Note that\n" );
  printf( "                                  a period (.) should be specified.\n" );
  printf( "      -o <filename>             Name of file to output ranking information to.  Default is stdout.\n" );
  printf( "      -weight-line <number>     Specifies a relative weighting for line coverage used to rank\n" );
  printf( "                                  non-unique coverage points.  A value of 0 removes line coverage\n" );
  printf( "                                  from ranking consideration.  Default value is 1.\n" );
  printf( "      -weight-toggle <number>   Specifies a relative weighting for toggle coverage used to rank\n" );
  printf( "                                  non-unique coverage points.  A value of 0 removes toggle coverage\n" );
  printf( "                                  from ranking consideration.  Default value is 1.\n" );
  printf( "      -weight-memory <number>   Specifies a relative weighting for memory coverage used to rank\n" );
  printf( "                                  non-unique coverage points.  A value of 0 removes memory coverage\n" );
  printf( "                                  from ranking consideration.  Default value is 1.\n" );
  printf( "      -weight-comb <number>     Specifies a relative weighting for combinational logic coverage used\n" );
  printf( "                                  to rank non-unique coverage points.  A value of 0 removes combinational\n" );
  printf( "                                  logic coverage from ranking consideration.  Default value is 1.\n" );
  printf( "      -weight-fsm <number>      Specifies a relative weighting for FSM state/state transition coverage\n" );
  printf( "                                  used to rank non-unique coverage points.  A value of 0 removes FSM\n" );
  printf( "                                  coverage from ranking consideration.  Default value is 1.\n" );
  printf( "      -weight-assert <number>   Specifies a relative weighting for assertion coverage used to rank\n" );
  printf( "                                  non-unique coverage points.  A value of 0 removes assertion coverage\n" );
  printf( "                                  from ranking consideration.  Default value is 0.\n" );
  printf( "      -v                        Outputs verbose information during the rank selection process.  This output\n" );
  printf( "                                  is not for debugging purposes, but rather gives the user insight into\n" );
  printf( "                                  what's going on \"behind the scenes\" during the ranking process.\n" );
  printf( "\n" );

}

/*!
 \return Returns TRUE if the help option was parsed.

 \throws anonymous Throw Throw Throw

 Parses the score argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the rank operation, an error message is displayed to the
 user.
*/
static bool rank_parse_args(
  int          argc,      /*!< Number of arguments in argument list argv */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< Argument list passed to this program */
) {

  int       i;
  unsigned  rank_in_num = 0;
  str_link* strl;
  str_link* ext_head    = NULL;
  str_link* ext_tail    = NULL;
  str_link* dir_head    = NULL;
  str_link* dir_tail    = NULL;
  bool      help_found  = FALSE;

  i = last_arg + 1;

  while( (i < argc) && !help_found ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      rank_usage();
      help_found = TRUE;

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( rank_file != NULL ) {
          print_output( "Only one -o option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( is_legal_filename( argv[i] ) ) {
            rank_file = strdup_safe( argv[i] );
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

    } else if( strncmp( "-f", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        char**       arg_list = NULL;
        int          arg_num  = 0;
        unsigned int j;
        i++;
        Try {
          read_command_file( argv[i], &arg_list, &arg_num );
          help_found = rank_parse_args( arg_num, -1, (const char**)arg_list );
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

    } else if( strncmp( "-required-list", argv[i], 14 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( file_exists( argv[i] ) ) {
          FILE* file;
          if( (file = fopen( argv[i], "r" )) != NULL ) {
            char         fname[4096];
            unsigned int rv;
            while( fscanf( file, "%s", fname ) == 1 ) {
              if( file_exists( fname ) ) {
                str_link* strl;
                if( (strl = str_link_find( fname, rank_in_head )) == NULL ) {
                  strl = str_link_add( strdup_safe( fname ), &rank_in_head, &rank_in_tail );
                }
                strl->suppl = 1;
              } else {
                unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Filename (%s) specified in -required file (%s) does not exist", fname, argv[i] );
                assert( rv < USER_MSG_LENGTH );
                print_output( user_msg, FATAL, __FILE__, __LINE__ );
                Throw 0;
              }
            }
            rv = fclose( file );
            assert( rv == 0 );
          } else {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to read -required-list file (%s)", argv[i] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Filename specified for -required-list option (%s) does not exist", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      
      } else {
        Throw 0;
      }

    } else if( strncmp( "-required-cdd", argv[i], 13 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( file_exists( argv[i] ) ) {
          str_link* strl;
          if( (strl = str_link_find( argv[i], rank_in_head )) == NULL ) {
            strl = str_link_add( strdup_safe( argv[i] ), &rank_in_head, &rank_in_tail );
          }
          strl->suppl = 1;
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to read -required-cdd file (%s)", argv[i] );
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

    } else if( strncmp( "-weight-line", argv[i], 12 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cdd_type_set[CP_TYPE_LINE] ) {
          print_output( "Only one -weight-line option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( sscanf( argv[i], "%u", &cdd_type_weight[CP_TYPE_LINE] ) != 1 ) {
            print_output( "Value specified after -weight-line must be a number", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            cdd_type_set[CP_TYPE_LINE] = TRUE;
          }
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-weight-toggle", argv[i], 14 ) == 0 ) {
  
      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cdd_type_set[CP_TYPE_TOGGLE] ) {
          print_output( "Only one -weight-toggle option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( sscanf( argv[i], "%u", &cdd_type_weight[CP_TYPE_TOGGLE] ) != 1 ) {
            print_output( "Value specified after -weight-toggle must be a number", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            cdd_type_set[CP_TYPE_TOGGLE] = TRUE;
          }
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-weight-memory", argv[i], 14 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cdd_type_set[CP_TYPE_MEM] ) {
          print_output( "Only one -weight-memory option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( sscanf( argv[i], "%u", &cdd_type_weight[CP_TYPE_MEM] ) != 1 ) {
            print_output( "Value specified after -weight-memory must be a number", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            cdd_type_set[CP_TYPE_MEM] = TRUE;
          }
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-weight-comb", argv[i], 12 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++; 
        if( cdd_type_set[CP_TYPE_LOGIC] ) {
          print_output( "Only one -weight-comb option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( sscanf( argv[i], "%u", &cdd_type_weight[CP_TYPE_LOGIC] ) != 1 ) {
            print_output( "Value specified after -weight-comb must be a number", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            cdd_type_set[CP_TYPE_LOGIC] = TRUE;
          }
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-weight-fsm", argv[i], 11 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cdd_type_set[CP_TYPE_FSM] ) {
          print_output( "Only one -weight-fsm option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( sscanf( argv[i], "%u", &cdd_type_weight[CP_TYPE_FSM] ) != 1 ) {
            print_output( "Value specified after -weight-fsm must be a number", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            cdd_type_set[CP_TYPE_FSM] = TRUE;
          }
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-weight-assert", argv[i], 14 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cdd_type_set[CP_TYPE_ASSERT] ) {
          print_output( "Only one -weight-assert option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( sscanf( argv[i], "%u", &cdd_type_weight[CP_TYPE_ASSERT] ) != 1 ) {
            print_output( "Value specified after -weight-assert must be a number", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            cdd_type_set[CP_TYPE_ASSERT] = TRUE;
          }
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-names-only", argv[i], 11 ) == 0 ) {

      flag_names_only = TRUE;

    } else if( strncmp( "-depth", argv[i], 6 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cp_depth != 0 ) {
          print_output( "Only one -depth option is allowed on the rank command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          if( (sscanf( argv[i], "%u", &cp_depth ) != 1) || (cp_depth == 0) ) {
            print_output( "Value specified after -depth must be a positive, non-zero number", FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
        }
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

    } else if( strncmp( "-v", argv[i], 2 ) == 0 ) {

      rank_verbose = TRUE;

    } else if( strncmp( "-", argv[i], 1 ) == 0 ) {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown rank option (%s) specified.", argv[i] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      /* The name of a file to rank */
      if( file_exists( argv[i] ) ) {

        if( str_link_find( argv[i], rank_in_head ) == NULL ) {

          /* Add the specified rank file to the list */
          (void)str_link_add( strdup_safe( argv[i] ), &rank_in_head, &rank_in_tail );

        }

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

      /* Load any ranking files found in specified directories */
      strl = dir_head;
      while( strl != NULL ) {
        directory_load( strl->str, ext_head, &rank_in_head, &rank_in_tail );
        strl = strl->next;
      }

    } Catch_anonymous {
      str_link_delete_list( ext_head );
      str_link_delete_list( dir_head );
      Throw 0;
    }

    /* Count the number of files being ranked */
    strl = rank_in_head;
    while( strl != NULL ) {
      rank_in_num++;
      strl = strl->next;
    }

    /* Check to make sure that the user specified at least two files to rank */
    if( rank_in_num < 2 ) {
      print_output( "Must specify at least two CDD files to rank", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }

    /* If no -depth option was specified, set its value to 1 */
    if( cp_depth == 0 ) {
      cp_depth = 1;
    }

  }

  /* Deallocate the temporary lists */
  str_link_delete_list( ext_head );
  str_link_delete_list( dir_head );

  return( help_found );

}

/*!
 Checks to make sure that index that will be used exceeds the maximum index (to eliminate memory
 overruns).  Throws exception if the index that will be used violates this check.
*/
static void rank_check_index(
  unsigned type,   /*!< Type of index that is being checked */
  uint64   index,  /*!< Index value to check */
  int      line    /*!< Line number in source code that hit this error */
) { PROFILE(RANK_CHECK_INDEX);

  if( index >= num_cps[type] ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Last read in CDD file is incompatible with previously read in CDD files.  Exiting..." );
    print_output( user_msg, FATAL, __FILE__, line );
    Throw 0;
  }

  PROFILE_END;

}

/*!
 Gathers all coverage point information from the given signal and populates the comp_cov structure accordingly.
*/
static void rank_gather_signal_cov(
  vsignal* sig,           /*!< Pointer to signal to gather coverage information from */
  comp_cdd_cov* comp_cov  /*!< Pointer to compressed CDD coverage structure to populate */
) { PROFILE(RANK_GATHER_SIGNAL_COV);

  /* Populate toggle coverage information */
  if( (sig->suppl.part.type != SSUPPL_TYPE_PARAM)      &&
      (sig->suppl.part.type != SSUPPL_TYPE_PARAM_REAL) &&
      (sig->suppl.part.type != SSUPPL_TYPE_ENUM)       &&
      (sig->suppl.part.type != SSUPPL_TYPE_MEM)        &&
      (sig->suppl.part.mba == 0) ) {

    unsigned int i;
    if( sig->suppl.part.excluded == 1 ) {
      for( i=0; i<sig->value->width; i++ ) {
        uint64 index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
        rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
        comp_cov->cps[CP_TYPE_TOGGLE][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
        index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
        rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
        comp_cov->cps[CP_TYPE_TOGGLE][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
      }
    } else {
      switch( sig->value->suppl.part.data_type ) {
        case VDATA_UL :
          for( i=0; i<sig->value->width; i++ ) {
            uint64 index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
            rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
            comp_cov->cps[CP_TYPE_TOGGLE][UL_DIV(index)] |= ((sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_SIG_TOG01] >> UL_MOD(i)) & (ulong)0x1) << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
            rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
            comp_cov->cps[CP_TYPE_TOGGLE][UL_DIV(index)] |= ((sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_SIG_TOG10] >> UL_MOD(i)) & (ulong)0x1) << UL_MOD(index);
          }
        break;
        default :  assert( 0 );  break;
      }
    }

  }

  /* Populate memory coverage information */
  if( (sig->suppl.part.type == SSUPPL_TYPE_MEM) && (sig->udim_num > 0) ) {

    unsigned int i;
    unsigned int pwidth = 1;

    for( i=(sig->udim_num); i<(sig->udim_num + sig->pdim_num); i++ ) {
      if( sig->dim[i].msb > sig->dim[i].lsb ) {
        pwidth *= (sig->dim[i].msb - sig->dim[i].lsb) + 1;
      } else {
        pwidth *= (sig->dim[i].lsb - sig->dim[i].msb) + 1;
      }
    }

    /* Calculate total number of addressable elements and their write/read information */
    for( i=0; i<sig->value->width; i+=pwidth ) {
      if( sig->suppl.part.excluded == 1 ) {
        uint64 index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
      } else {
        unsigned int wr = 0;
        unsigned int rd = 0;
        uint64       index;
        vector_mem_rw_count( sig->value, (int)i, (int)((i + pwidth) - 1), &wr, &rd );
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= (ulong)((wr > 0) ? 1 : 0) << UL_MOD(index);
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= (ulong)((rd > 0) ? 1 : 0) << UL_MOD(index);
      }
    }

    /* Calculate toggle coverage information for the memory */
    if( sig->suppl.part.excluded == 1 ) {
      for( i=0; i<sig->value->width; i++ ) {
        uint64 index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
      }
    } else {
      switch( sig->value->suppl.part.data_type ) {
        case VDATA_UL :
          for( i=0; i<sig->value->width; i++ ) {  
            uint64 index = comp_cov->cps_index[CP_TYPE_MEM]++;
            rank_check_index( CP_TYPE_MEM, index, __LINE__ );
            comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= ((sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_MEM_TOG01] >> UL_MOD(i)) & (ulong)0x1) << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_MEM]++;
            rank_check_index( CP_TYPE_MEM, index, __LINE__ );
            comp_cov->cps[CP_TYPE_MEM][UL_DIV(index)] |= ((sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_MEM_TOG10] >> UL_MOD(i)) & (ulong)0x1) << UL_MOD(index);
          }
          break;
        default :  assert( 0 );  break;
      } 
    }   
        
  }   

  PROFILE_END;

}

/*!
 Recursively iterates through the given expression tree, gathering all combinational logic coverage information and
 populating the given compressed CDD coverage structure accordingly.
*/
static void rank_gather_comb_cov(
            expression*   exp,      /*!< Pointer to current expression to gather combinational logic coverage from */
  /*@out@*/ comp_cdd_cov* comp_cov  /*!< Pointer to compressed CDD coverage structure to populate */
) { PROFILE(RANK_GATHER_COMB_COV);

  if( exp != NULL ) {

    /* Gather combination coverage information from children */
    rank_gather_comb_cov( exp->left,  comp_cov );
    rank_gather_comb_cov( exp->right, comp_cov );

    /* Calculate combinational logic coverage information */
    if( (EXPR_IS_MEASURABLE( exp ) == 1) && (ESUPPL_WAS_COMB_COUNTED( exp->suppl ) == 0) ) {
      
      /* Calculate current expression combination coverage */
      if( !expression_is_static_only( exp ) ) {
    
        uint64 index;

        if( EXPR_IS_COMB( exp ) == 1 ) {
          if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)(exp->suppl.part.eval_00 | exp->suppl.part.eval_01) << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)(exp->suppl.part.eval_00 | exp->suppl.part.eval_10) << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)exp->suppl.part.eval_11 << UL_MOD(index);
          } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)(exp->suppl.part.eval_11 | exp->suppl.part.eval_10) << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)(exp->suppl.part.eval_11 | exp->suppl.part.eval_01) << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)exp->suppl.part.eval_00 << UL_MOD(index);
          } else if( exp_op_info[exp->op].suppl.is_comb == OTHER_COMB ) {
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)exp->suppl.part.eval_00 << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)exp->suppl.part.eval_01 << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)exp->suppl.part.eval_10 << UL_MOD(index);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)exp->suppl.part.eval_11 << UL_MOD(index);
          }
        } else if( EXPR_IS_EVENT( exp ) == 1 ) {
          index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
          rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
          comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)ESUPPL_WAS_TRUE( exp->suppl ) << UL_MOD(index);
        } else {
          index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
          rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
          comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)ESUPPL_WAS_TRUE( exp->suppl ) << UL_MOD(index);
          index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
          rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
          comp_cov->cps[CP_TYPE_LOGIC][UL_DIV(index)] |= (ulong)ESUPPL_WAS_FALSE( exp->suppl ) << UL_MOD(index);
        }
  
      }
  
    }

    /* Set the counted bit */
    exp->suppl.part.comb_cntd = 1;

  }

  PROFILE_END;

}

/*!
 Gathers line and combinational logic coverage point information from the given expression and populates
 the specified compressed CDD coverage structure accordingly.
*/
static void rank_gather_expression_cov(
            expression*   exp,      /*!< Pointer to expression to gather coverage information from */
            unsigned int  exclude,  /*!< Specifies whether line coverage information should be excluded */
  /*@out@*/ comp_cdd_cov* comp_cov  /*!< Pointer to compressed CDD coverage structure to populate */
) { PROFILE(RANK_GATHER_EXPRESSION_COV);

  /* Calculate line coverage information (NOTE:  we currently ignore the excluded status of the line) */
  if( (exp->suppl.part.root == 1) &&
      (exp->op != EXP_OP_DELAY)   &&
      (exp->op != EXP_OP_CASE)    &&
      (exp->op != EXP_OP_CASEX)   &&
      (exp->op != EXP_OP_CASEZ)   &&
      (exp->op != EXP_OP_DEFAULT) &&
      (exp->op != EXP_OP_NB_CALL) &&
      (exp->op != EXP_OP_FORK)    &&
      (exp->op != EXP_OP_JOIN)    &&
      (exp->op != EXP_OP_NOOP)    &&
      (exp->line != 0) ) {
    uint64 index = comp_cov->cps_index[CP_TYPE_LINE]++;
    rank_check_index( CP_TYPE_LINE, index, __LINE__ );
    if( (exp->exec_num > 0) || exclude ) {
      comp_cov->cps[CP_TYPE_LINE][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
    }
  }

  /* Calculate combinational logic coverage information */
  rank_gather_comb_cov( exp, comp_cov );

  PROFILE_END;

}

/*!
 Gathers the FSM coverage points from the given FSM and populates the compressed CDD coverage structure
 accordingly.
*/
static void rank_gather_fsm_cov(
  fsm_table*    table,    /*!< Pointer to FSM table to gather coverage information from */
  comp_cdd_cov* comp_cov  /*!< Pointer to compressed CDD coverage structure to populate */
) { PROFILE(RANK_GATHER_FSM_COV);

  /* We can only create a compressed version of FSM coverage information if it is known */
  if( table->suppl.part.known == 0 ) {

    int*         state_hits = (int*)malloc_safe( sizeof( int ) * table->num_fr_states );
    unsigned int i;

    /* Initialize state_hits array */
    for( i=0; i<table->num_fr_states; i++ ) {
      state_hits[i] = 0;
    }

    /* Iterate through arc transition array and count unique hits */
    for( i=0; i<table->num_arcs; i++ ) {
      uint64 index;
      if( (table->arcs[i]->suppl.part.hit || table->arcs[i]->suppl.part.excluded) ) {
        if( state_hits[table->arcs[i]->from]++ == 0 ) {
          index = comp_cov->cps_index[CP_TYPE_FSM]++;
          rank_check_index( CP_TYPE_FSM, index, __LINE__ );
          comp_cov->cps[CP_TYPE_FSM][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
        }
        index = comp_cov->cps_index[CP_TYPE_FSM]++;
        rank_check_index( CP_TYPE_FSM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_FSM][UL_DIV(index)] |= (ulong)0x1 << UL_MOD(index);
      } else {
        if( state_hits[table->arcs[i]->from]++ == 0 ) {
          index = comp_cov->cps_index[CP_TYPE_FSM]++;
          rank_check_index( CP_TYPE_FSM, index, __LINE__ );
        }
        index = comp_cov->cps_index[CP_TYPE_FSM]++;
        rank_check_index( CP_TYPE_FSM, index, __LINE__ );
      }
    }

    /* Deallocate state_hits array */
    free_safe( state_hits, (sizeof( int ) * table->num_fr_states) );

  }

  PROFILE_END;

}

/*!
 Recursively iterates through the instance tree, accumulating values for num_cps array.
*/
static void rank_calc_num_cps(
            funit_inst* inst,              /*!< Pointer to instance tree to calculate num_cps array */
  /*@out@*/ uint64      nums[CP_TYPE_NUM]  /*!< Array of coverage point numbers to populate */
) { PROFILE(RANK_CALC_NUM_CPS);

  funit_inst* child;  /* Pointer to child instance */

  /* Iterate through children instances */
  child = inst->child_head;
  while( child != NULL ) {
    rank_calc_num_cps( child, nums );
    child = child->next;
  }

  /* Add totals to global num_cps array */
  nums[CP_TYPE_LINE]   += inst->stat->line_total;
  nums[CP_TYPE_TOGGLE] += (inst->stat->tog_total * 2);
  nums[CP_TYPE_MEM]    += (inst->stat->mem_ae_total * 2) + (inst->stat->mem_tog_total * 2);
  nums[CP_TYPE_LOGIC]  += inst->stat->comb_total;
  if( inst->stat->state_total > 0 ) {
    nums[CP_TYPE_FSM] += (unsigned int)inst->stat->state_total;
  }
  if( inst->stat->arc_total > 0 ) {
    nums[CP_TYPE_FSM] += (unsigned int)inst->stat->arc_total;
  }
  nums[CP_TYPE_ASSERT] += inst->stat->assert_total;

  PROFILE_END;

}

/*!
 Gathers all coverage point information from the given functional unit instance and populates
 the specified compressed CDD coverage structure accordingly.
*/
static void rank_gather_comp_cdd_cov(
  funit_inst*   inst,
  comp_cdd_cov* comp_cov
) { PROFILE(RANK_GATHER_COMP_CDD_COV);

  funit_inst* child;  /* Pointer to current child instance */

  /* Don't gather information for placeholder instances */
  if( inst->funit != NULL ) {

    unsigned int i;

    /* Gather coverage information from expressions */
    if( !funit_is_unnamed( inst->funit ) ) {
      func_iter  fi;
      statement* stmt;

      /* First, clear the comb_cntd bits in all of the expressions */
      func_iter_init( &fi, inst->funit, TRUE, FALSE, FALSE );
      while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {
        combination_reset_counted_expr_tree( stmt->exp );
      }
      func_iter_dealloc( &fi );

      /* Then populate the comp_cov structure, accordingly */
      func_iter_init( &fi, inst->funit, TRUE, FALSE, FALSE );
      while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {
        rank_gather_expression_cov( stmt->exp, stmt->suppl.part.excluded, comp_cov );
      }
      func_iter_dealloc( &fi );
    }

    /* Gather coverage information from signals */
    for( i=0; i<inst->funit->sig_size; i++ ) {
      rank_gather_signal_cov( inst->funit->sigs[i], comp_cov );
    }

    /* Gather coverage information from FSMs */
    for( i=0; i<inst->funit->fsm_size; i++ ) {
      rank_gather_fsm_cov( inst->funit->fsms[i]->table, comp_cov );
    }

  }

  /* Gather coverage information from children */
  child = inst->child_head;
  while( child != NULL ) {
    rank_gather_comp_cdd_cov( child, comp_cov );
    child = child->next;
  }

  PROFILE_END;

}

/*!
 Parses the given CDD name and stores its coverage point information in a compressed format.
*/
static void rank_read_cdd(
            const char*     cdd_name,     /*!< Filename of CDD file to read in */
            bool            required,     /*!< Specifies if CDD file is required to be ranked */
            bool            first,        /*!< Set to TRUE if this if the first CDD being read */
  /*@out@*/ comp_cdd_cov*** comp_cdds,    /*!< Pointer to compressed CDD array */
  /*@out@*/ unsigned int*   comp_cdd_num  /*!< Number of compressed CDD structures in comp_cdds array */
) { PROFILE(RANK_READ_CDD);

  comp_cdd_cov* comp_cov = NULL;

  Try {

    inst_link* instl;
    uint64     tmp_nums[CP_TYPE_NUM] = {0};

    /* Read in database */
    (void)db_read( cdd_name, READ_MODE_REPORT_NO_MERGE );
    bind_perform( TRUE, 0 );

    /* Calculate the num_cps array if we are the first or check our coverage points to verify that they match */
    instl = db_list[0]->inst_head;
    while( instl != NULL ) {
      report_gather_instance_stats( instl->inst );
      if( first ) {
        rank_calc_num_cps( instl->inst, num_cps );
      } else {
        rank_calc_num_cps( instl->inst, tmp_nums );
      }
      instl = instl->next;
    }

    /* If we are not the first CDD file being read in, verify that our values match */
    if( !first ) {
      unsigned int i;
      for( i=0; i<CP_TYPE_NUM; i++ ) {
        if( num_cps[i] != tmp_nums[i] ) {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "CDD file \"%s\" does not match previously read CDD files", cdd_name );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      }
    }

    /* Allocate the memory needed for the compressed CDD coverage structure */
    comp_cov = rank_create_comp_cdd_cov( cdd_name, required, num_timesteps );

    /* Finally, populate compressed CDD coverage structure with coverage information from database signals */
    instl = db_list[0]->inst_head;
    while( instl != NULL ) {
      rank_gather_comp_cdd_cov( instl->inst, comp_cov );
      instl = instl->next;
    }

    /* Add compressed CDD coverage structure to array */
    *comp_cdds = (comp_cdd_cov**)realloc_safe( *comp_cdds, (sizeof( comp_cdd_cov* ) * (*comp_cdd_num)), (sizeof( comp_cdd_cov* ) * (*comp_cdd_num + 1)) );
    (*comp_cdds)[*comp_cdd_num] = comp_cov;
    (*comp_cdd_num)++;

  } Catch_anonymous {
    db_close();
    rank_dealloc_comp_cdd_cov( comp_cov );
    Throw 0;
  }

  /* Close the database */
  db_close();

  PROFILE_END;

}

/*-----------------------------------------------------------------------------------------------------------------------*/

/*!
 Sorts the selected CDD coverage structure into the comp_cdds list and performs post-placement calculations.
*/
static void rank_selected_cdd_cov(
  /*@out@*/ comp_cdd_cov** comp_cdds,        /*!< Pointer to array of compressed CDD coverage structures being sorted */
            unsigned int   comp_cdd_num,     /*!< Total number of elements in comp_cdds array */
  /*@out@*/ uint16*        ranked_merged,    /*!< Array of merged information for ranked CDDs */
  /*@out@*/ uint16*        unranked_merged,  /*!< Array of merged information for unranked CDDs */
            unsigned int   next_cdd,         /*!< Index into comp_cdds array that the selected CDD should be stored at */
            unsigned int   selected_cdd      /*!< Index into comp_cdds array of the selected CDD for ranking */
) { PROFILE(RANK_SELECTED_CDD_COV);

  unsigned int        i, j;
  uint64              merged_index = 0;
  static unsigned int dots_output  = 0;
  comp_cdd_cov*       tmp;

  /* Output status indicator, if necessary */
  if( ((!quiet_mode && !terse_mode) || debug_mode) && !rank_verbose ) {
    while( ((unsigned int)(((next_cdd + 1) / (float)comp_cdd_num) * 100) - (dots_output * 10)) >= 10 ) { 
      unsigned int rv;
      printf( "." );
      rv = fflush( stdout );
      assert( rv == 0 );
      dots_output++;
    }
  }

  /* Move the most unique CDD to the next position */
  tmp                     = comp_cdds[next_cdd];
  comp_cdds[next_cdd]     = comp_cdds[selected_cdd];
  comp_cdds[selected_cdd] = tmp;

  /* Zero out uniqueness value */
  comp_cdds[next_cdd]->unique_cps = 0;

  /* Subtract all of the set coverage points from the merged value */
  for( i=0; i<CP_TYPE_NUM; i++ ) {
    for( j=0; j<num_cps[i]; j++ ) {
      if( unranked_merged[merged_index] > 0 ) {
        if( comp_cdds[next_cdd]->cps[i][UL_DIV(j)] & ((ulong)0x1 << UL_MOD(j)) ) {
          /*
           If we have not seen this coverage point get hit the needed "depth" amount in the ranked
           list, increment the unique_cps value for the selected compressed CDD coverage structure.
          */
          if( ranked_merged[merged_index] < cp_depth ) {
            comp_cdds[next_cdd]->unique_cps++;
          }
          unranked_merged[merged_index]--;
          ranked_merged[merged_index]++;
        }
      }
      merged_index++;
    }
  }

  if( ((!quiet_mode && !terse_mode) || debug_mode) && !rank_verbose ) {
    if( (next_cdd + 1) == comp_cdd_num ) {
      unsigned int rv;
      if( dots_output < 10 ) {
        printf( "." );
      }
      printf( "\n" );
      rv = fflush( stdout );
      assert( rv == 0 );
    }
  }

  PROFILE_END;

}

/*!
 Performs ranking according to scores that are calculated from the user-specified weights and the amount of
 coverage points left to be hit.  Ranks all compressed CDD coverage structures between next_cdd and the end of
 the array (comp_cdd_num - 1), inclusive.
*/
static void rank_perform_weighted_selection(
  /*@out@*/ comp_cdd_cov** comp_cdds,        /*!< Reference to partially sorted list of compressed CDD coverage structures to sort */
            unsigned int   comp_cdd_num,     /*!< Number of compressed CDD coverage structures in the comp_cdds array */
            uint16*        ranked_merged,    /*!< Array of ranked merged information from all of the compressed CDD coverage structures */
            uint16*        unranked_merged,  /*!< Array of unranked merged information from all of the compressed CDD coverage structures */
            unsigned int   next_cdd,         /*!< Next index in comp_cdds array to set */
  /*@out@*/ unsigned int*  cdds_ranked       /*!< Number of CDDs that were ranked with unique coverage in this function */
) { PROFILE(RANK_PERFORM_WEIGHTED_SELECTION);

  /* Perform this loop for each remaining coverage file */
  for( ; next_cdd<comp_cdd_num; next_cdd++ ) {

    unsigned int i, j, k;
    unsigned int highest_score = next_cdd;

    /* Calculate current scores */
    for( i=next_cdd; i<comp_cdd_num; i++ ) {
      bool   unique_found = FALSE;
      uint64 x            = 0;
      comp_cdds[i]->score = 0;
      for( j=0; j<CP_TYPE_NUM; j++ ) {
        unsigned int total = 0;
        for( k=0; k<num_cps[j]; k++ ) {
          if( unranked_merged[x] > 0 ) {
  	    if( comp_cdds[i]->cps[j][UL_DIV(k)] & ((ulong)0x1 << UL_MOD(k)) ) {
              total++;
              if( ranked_merged[x] < cp_depth ) {
                unique_found = TRUE;
              }
            }
          }
          x++;
        }
        comp_cdds[i]->score += ((total / (float)comp_cdds[i]->timesteps) * 100) * cdd_type_weight[j];
      }
      if( (comp_cdds[i]->score > comp_cdds[highest_score]->score) && unique_found ) {
        highest_score = i;
      }
    } 

    /* Store the selected CDD into the next slot of the comp_cdds array */
    rank_selected_cdd_cov( comp_cdds, comp_cdd_num, ranked_merged, unranked_merged, next_cdd, highest_score );

    /* Increment the number of unique_cps ranked */
    if( comp_cdds[next_cdd]->unique_cps > 0 ) {
      (*cdds_ranked)++;
    }

  }

  PROFILE_END;

}

/*!
 Re-sorts the compressed CDD coverage array to order them based on a "most coverage points per timestep" basis.
*/
static void rank_perform_greedy_sort(
  /*@out@*/ comp_cdd_cov** comp_cdds,      /*!< Pointer to compressed CDD coverage structure array to re-sort */
            unsigned int   comp_cdd_num,   /*!< Number of elements in comp-cdds array */
            uint16*        ranked_merged,  /*!< Array for recalculating uniqueness of sorted elements */
            uint64         num_ranked      /*!< Number of elements in ranked_merged array */
) { PROFILE(RANK_PERFORM_GREEDY_SORT);

  unsigned int  i, j, k, l;
  unsigned int  best;
  uint64        x;
  comp_cdd_cov* tmp;

  /* First, reset the ranked_merged array */
  for( x=0; x<num_ranked; x++ ) {
    ranked_merged[x] = 0;
  }

  /* Rank based on most unique from previously ranked CDDs */
  for( i=0; i<comp_cdd_num; i++ ) {
    best = i;
    for( j=i; j<comp_cdd_num; j++ ) {
      x = 0;
      comp_cdds[j]->unique_cps = 0;
      for( k=0; k<CP_TYPE_NUM; k++ ) {
        for( l=0; l<num_cps[k]; l++ ) {
          if( comp_cdds[j]->cps[k][UL_DIV(l)] & ((ulong)0x1 << UL_MOD(l)) ) {
            if( ranked_merged[x] < cp_depth ) {
              comp_cdds[j]->unique_cps++;
            }
          }
          x++;
        }
      }
      if( (comp_cdds[best]->unique_cps < comp_cdds[j]->unique_cps) ||
          ((comp_cdds[best]->unique_cps == comp_cdds[j]->unique_cps) && (comp_cdds[best]->timesteps < comp_cdds[j]->timesteps)) ||
          ((comp_cdds[best]->unique_cps == 0) && !comp_cdds[best]->required && !comp_cdds[i]->required) ) {
        best = j;
      }
    }
    tmp             = comp_cdds[i];
    comp_cdds[i]    = comp_cdds[best];
    comp_cdds[best] = tmp;
    x = 0;
    for( j=0; j<CP_TYPE_NUM; j++ ) {
      for( k=0; k<num_cps[j]; k++ ) {
        if( comp_cdds[i]->cps[j][UL_DIV(k)] & ((ulong)0x1 << UL_MOD(k)) ) {
          ranked_merged[x]++;
        }
        x++;
      }
    }
  }

  PROFILE_END;

}

/*!
 \return Returns the number of coverage points hit in the given list.
*/
uint64 rank_count_cps(
  uint16*      list,      /*!< List of cp counts */
  unsigned int list_size  /*!< Number of elements in the list */
) { PROFILE(RANK_COUNT_CPS);

  uint64       cps = 0;
  unsigned int i;

  for( i=0; i<list_size; i++ ) {
    cps += (list[i] > 0) ? 1 : 0;
  }

  PROFILE_END;

  return( cps );

}

/*!
 Performs the task of ranking the CDD files and rearranging them in the comp_cdds array such that the
 first CDD file is located at index 0.
*/
static void rank_perform(
  /*@out@*/ comp_cdd_cov** comp_cdds,    /*!< Pointer to array of compressed CDD coverage structures to rank */
            unsigned int   comp_cdd_num  /*!< Number of allocated structures in comp_cdds array */
) { PROFILE(RANK_PERFORM);

  unsigned int i, j, k;
  uint16*      ranked_merged;
  uint16*      unranked_merged;
  uint16       merged_index = 0;
  uint64       total        = 0;
  uint64       total_hitable;
  unsigned int next_cdd     = 0;
  unsigned int most_unique;
  unsigned int count;
  unsigned int cdds_ranked  = 0;
  timer*       atimer       = NULL;
  unsigned int rv;

  if( ((!quiet_mode && !terse_mode) || debug_mode) && !rank_verbose ) {
    printf( "Ranking CDD files " );
    rv = fflush( stdout );
    assert( rv == 0 );
  }

  /* Calculate the total number of needed merged entries to store accumulated information */
  for( i=0; i<CP_TYPE_NUM; i++ ) {
    total += num_cps[i];
  }
  assert( total > 0 );

  /* Allocate merged array */
  ranked_merged   = (uint16*)calloc_safe( total, sizeof( uint16 ) );
  unranked_merged = (uint16*)malloc_safe_nolimit( sizeof( uint16 ) * total );

  if( rank_verbose ) {
    /*@-duplicatequals -formattype -formatcode@*/
    rv = snprintf( user_msg, USER_MSG_LENGTH, "\nRanking %u CDD files with %" FMT64 "u coverage points (%" FMT64 "u line, %" FMT64 "u toggle, %" FMT64 "u memory, %" FMT64 "u logic, %" FMT64 "u FSM, %" FMT64 "u assertion)",
              comp_cdd_num, total, num_cps[CP_TYPE_LINE], num_cps[CP_TYPE_TOGGLE], num_cps[CP_TYPE_MEM], num_cps[CP_TYPE_LOGIC], num_cps[CP_TYPE_FSM], num_cps[CP_TYPE_ASSERT] );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
  }

  /* Step 1 - Initialize merged results array, calculate uniqueness and total values of each compressed CDD coverage structure */
  for( i=0; i<CP_TYPE_NUM; i++ ) {
    for( j=0; j<num_cps[i]; j++ ) {
      uint16       bit_total = 0;
      unsigned int set_cdd;
      for( k=0; k<comp_cdd_num; k++ ) {
        if( (comp_cdds[k]->cps[i][UL_DIV(j)] & ((ulong)0x1 << UL_MOD(j))) != 0 ) {
          comp_cdds[k]->total_cps++;
          set_cdd = k;
          bit_total++;
        }
      }
      unranked_merged[merged_index++] = bit_total;

      /* If we found exactly one CDD file that hit this coverage point, mark it in the corresponding CDD file */
      if( bit_total == 1 ) {
        comp_cdds[set_cdd]->unique_cps++;
      }
    }
  }

  if( rank_verbose ) {
    total_hitable = rank_count_cps( unranked_merged, total ); 
    /*@-duplicatequals +ignorequals -formatcode@*/
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Ignoring %" FMT64 "u coverage points that were not hit by any CDD file", (total - total_hitable) );
    /*@=duplicatequals =ignorequals =formatcode@*/
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );

    print_output( "\nPhase 1:  Adding user-required files", NORMAL, __FILE__, __LINE__ );
    rv = fflush( stdout );
    assert( rv == 0 );
    timer_clear( &atimer );
    timer_start( &atimer );
  }

  /* Step 2 - Immediately rank all of the required CDDs */
  for( i=0; i<comp_cdd_num; i++ ) {
    if( comp_cdds[i]->required ) {
      rank_selected_cdd_cov( comp_cdds, comp_cdd_num, ranked_merged, unranked_merged, next_cdd, i );
      next_cdd++;
    }
  }

  if( rank_verbose ) {
    uint64 ranked_cps = rank_count_cps( ranked_merged, total );
    timer_stop( &atimer );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "  Ranked %u CDD files (Total ranked: %u, Remaining: %u)", next_cdd, next_cdd, (comp_cdd_num - next_cdd) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    /*@-duplicatequals -formattype -formatcode@*/
    rv = snprintf( user_msg, USER_MSG_LENGTH, "  %" FMT64 "u points covered, %" FMT64 "u points remaining", ranked_cps, (total_hitable - ranked_cps) );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Completed phase 1 in %s", timer_to_string( atimer ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
  
    count = next_cdd;
    print_output( "\nPhase 2:  Adding files that hit unique coverage points", NORMAL, __FILE__, __LINE__ );
    rv = fflush( stdout );
    assert( rv == 0 );
    timer_clear( &atimer );
    timer_start( &atimer );
  }

  /* Step 3 - Start with the most unique CDDs */
  do {
    most_unique = next_cdd;
    for( i=(next_cdd+1); i<comp_cdd_num; i++ ) {
      if( comp_cdds[i]->unique_cps > comp_cdds[most_unique]->unique_cps ) {
        most_unique = i;
      }
    }
    if( comp_cdds[most_unique]->unique_cps > 0 ) {
      rank_selected_cdd_cov( comp_cdds, comp_cdd_num, ranked_merged, unranked_merged, next_cdd, most_unique );
      next_cdd++;
    }
  } while( (next_cdd < comp_cdd_num) && (comp_cdds[most_unique]->unique_cps > 0) );

  if( rank_verbose ) {
    uint64 ranked_cps = rank_count_cps( ranked_merged, total );
    timer_stop( &atimer );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "  Ranked %u CDD files (Total ranked: %u, Remaining: %u)", (next_cdd - count), next_cdd, (comp_cdd_num - next_cdd) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    /*@-duplicatequals -formattype -formatcode@*/
    rv = snprintf( user_msg, USER_MSG_LENGTH, "  %" FMT64 "u points covered, %" FMT64 "u points remaining", ranked_cps, (total_hitable - ranked_cps) );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Completed phase 2 in %s", timer_to_string( atimer ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
   
    count = next_cdd;
    print_output( "\nPhase 3:  Adding files that hit remaining coverage points and eliminating redundant files", NORMAL, __FILE__, __LINE__ );
    rv = fflush( stdout );
    assert( rv == 0 );
    timer_clear( &atimer );
    timer_start( &atimer );
  }

  /* Step 4 - Select coverage based on user-specified factors */
  if( next_cdd < comp_cdd_num ) {
    rank_perform_weighted_selection( comp_cdds, comp_cdd_num, ranked_merged, unranked_merged, next_cdd, &cdds_ranked );
  }

  if( rank_verbose ) {
    uint64 ranked_cps = rank_count_cps( ranked_merged, total );
    timer_stop( &atimer );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "  Ranked %u CDD files (Total ranked: %u, Eliminated: %u)", cdds_ranked, (count + cdds_ranked), (comp_cdd_num - (count + cdds_ranked)) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    /*@-duplicatequals -formattype -formatcode@*/
    rv = snprintf( user_msg, USER_MSG_LENGTH, "  %" FMT64 "u points covered, %" FMT64 "u points remaining", ranked_cps, (total_hitable - ranked_cps) );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Completed phase 3 in %s", timer_to_string( atimer ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );

    print_output( "\nPhase 4:  Sorting CDD files selected for ranking (no reductions)", NORMAL, __FILE__, __LINE__ );
    rv = fflush( stdout );
    assert( rv == 0 );
    timer_clear( &atimer );
    timer_start( &atimer );
  }

  /* Step 5 - Re-sort the list using a greedy algorithm */
  rank_perform_greedy_sort( comp_cdds, comp_cdd_num, ranked_merged, total );

  if( rank_verbose ) {
    timer_stop( &atimer );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Completed phase 4 in %s", timer_to_string( atimer ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    rv = fflush( stdout );
    assert( rv == 0 );
    free_safe( atimer, sizeof( timer ) );

    if( comp_cdd_num == (count + cdds_ranked) ) {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "\nSUMMARY:  No reduction occurred.  %u needed/required", (count + cdds_ranked) );
      assert( rv < USER_MSG_LENGTH );
    } else {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "\nSUMMARY:  Reduced %u CDD files down to %u needed/required", comp_cdd_num, (count + cdds_ranked) );
      assert( rv < USER_MSG_LENGTH );
    }
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
  }

  /* Deallocate merged CDD coverage structure */
  free_safe( ranked_merged,   (sizeof( uint16 ) * total ) );
  free_safe( unranked_merged, (sizeof( uint16 ) * total ) );

  PROFILE_END;

}

/*-----------------------------------------------------------------------------------------------------------------------*/

/*!
 Outputs the ranking of the CDD files to the output file specified from the rank command line.
*/
static void rank_output(
  comp_cdd_cov** comp_cdds,    /*!< Pointer to array of ranked CDD coverage structures */
  unsigned int   comp_cdd_num  /*!< Number of allocated structures in comp_cdds array */
) { PROFILE(RANK_OUTPUT);

  FILE*        ofile;
  unsigned int rv;

  if( rank_file == NULL ) {
    print_output( "\nGenerating report output to standard output...", NORMAL, __FILE__, __LINE__ );
  } else {
    rv = snprintf( user_msg, USER_MSG_LENGTH, "\nGenerating report file \"%s\"...", rank_file );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
  }

  if( (ofile = ((rank_file == NULL) ? stdout : fopen( rank_file, "w" ))) != NULL ) {

    unsigned int rv;
    unsigned int i;
    uint64       acc_timesteps  = 0;
    uint64       acc_unique_cps = 0;
    bool         unique_found   = TRUE;
    uint64       total_cps      = 0;

    /* Calculate the total number of coverage points */
    for( i=0; i<CP_TYPE_NUM; i++ ) {
      total_cps += num_cps[i];
    }

    /* If we are outputting to standard output, make sure we have a few newlines for readability purposes */
    if( ofile == stdout ) {
      fprintf( ofile, "\n\n\n" );
    }

    if( flag_names_only ) {

      for( i=0; i<comp_cdd_num; i++ ) {
        if( comp_cdds[i]->unique_cps > 0 ) {
          fprintf( ofile, "%s\n", comp_cdds[i]->cdd_name );
        }
      }

    } else {

      char* str;
      char  format[100];

      /* Allocate memory for a spacing string */
      str = (char*)malloc_safe( longest_name_len + 1 );

      /* Header information output */
      fprintf( ofile, "                                           ::::::::::::::::::::::::::::::::::::::::::::::::::::\n" );
      fprintf( ofile, "                                           ::                                                ::\n" );
      fprintf( ofile, "                                           ::     Covered -- Simulation Ranked Run Order     ::\n" );
      fprintf( ofile, "                                           ::                                                ::\n" );
      fprintf( ofile, "                                           ::::::::::::::::::::::::::::::::::::::::::::::::::::\n\n\n" );
      fprintf( ofile, "\n" );

      /* Calculate and display reduction status */
      i = 0;
      while( (i<comp_cdd_num) && ((comp_cdds[i]->unique_cps > 0) || comp_cdds[i]->required) ) i++;
      if( i == comp_cdd_num ) {
        fprintf( ofile, "No reduction occurred\n" );
      } else {
        uint64       total_timesteps  = 0;
        uint64       ranked_timesteps = 0;
        unsigned int col1, col2, j;
        char         str[30];
        char         fmt[4096];

        /* Calculated total_timesteps and ranked_timesteps */
        for( j=0; j<comp_cdd_num; j++ ) {
          total_timesteps += comp_cdds[j]->timesteps;
          if( (comp_cdds[j]->unique_cps > 0) || comp_cdds[j]->required ) {
            ranked_timesteps = total_timesteps;
          }
        }

        /* Figure out the largest number for the first column */
        /*@-duplicatequals -formattype -formatcode@*/
        rv = snprintf( str, 30, "%" FMT64 "u", total_timesteps );   col1 = strlen( str );
        assert( rv < 30 );
        rv = snprintf( str, 30, "%" FMT64 "u", ranked_timesteps );  col2 = strlen( str );
        /*@=duplicatequals =formattype =formatcode@*/
        assert( rv < 30 );

        /* Create line for CDD files */
        rv = snprintf( fmt, 4096, "* Reduced %%%uu CDD files down to %%%uu needed to maintain coverage (%%3.0f%%%% reduction, %%5.1fx improvement)\n", col1, col2 );
        assert( rv < 4096 );
        fprintf( ofile, fmt, comp_cdd_num, i, (((comp_cdd_num - i) / (float)comp_cdd_num) * 100), (comp_cdd_num / (float)i) );

        /* Create line for timesteps */
        rv = snprintf( fmt, 4096, "* Reduced %%%ullu timesteps down to %%%ullu needed to maintain coverage (%%3.0f%%%% reduction, %%5.1fx improvement)\n", col1, col2 );
        assert( rv < 4096 );
        fprintf( ofile, fmt, total_timesteps, ranked_timesteps, (((total_timesteps - ranked_timesteps) / (double)total_timesteps) * 100), (total_timesteps / (double)ranked_timesteps) );
      }
      fprintf( ofile, "\n" );
      gen_char_string( str, '-', (longest_name_len - 3) );
      fprintf( ofile, "-----------+-------------------------------------------+---------%s------------------------------------------\n", str );
      gen_char_string( str, ' ', (longest_name_len >> 1) );
      fprintf( ofile, "           |                ACCUMULATIVE               |         %s               CDD\n", str );
      gen_char_string( str, '-', (longest_name_len - 3) );
      fprintf( ofile, "Simulation |-------------------------------------------+---------%s------------------------------------------\n", str );
      gen_char_string( str, ' ', (longest_name_len - 3) );
      fprintf( ofile, "Order      |        Hit /      Total     %%   Timesteps |  R  Name%s        Hit /      Total     %%   Timesteps\n", str );
      gen_char_string( str, '-', (longest_name_len - 3) );
      fprintf( ofile, "-----------+-------------------------------------------+---------%s------------------------------------------\n", str );
      fprintf( ofile, "\n" );

      /* Calculate a string format */
      rv = snprintf( format, 100, "%%10u   %%10llu   %%10llu  %%3.0f%%%%  %%10llu    %%c  %%-%us  %%10llu   %%10llu  %%3.0f%%%%  %%10llu\n", longest_name_len );
      assert( rv < 100 );

      for( i=0; i<comp_cdd_num; i++ ) {
        acc_timesteps  += comp_cdds[i]->timesteps; 
        acc_unique_cps += comp_cdds[i]->unique_cps;
        if( (comp_cdds[i]->unique_cps == 0) && unique_found && !comp_cdds[i]->required ) {
          fprintf( ofile, "\n---------------------------------------  The following CDD files add no additional coverage  ----------------------------------------------\n\n" );
          unique_found = FALSE;
        }
        fprintf( ofile, format,
                (i + 1),
                acc_unique_cps,
                total_cps,
                ((acc_unique_cps / (float)total_cps) * 100),
                acc_timesteps,
                (comp_cdds[i]->required ? '*' : ' '),
                comp_cdds[i]->cdd_name,
                comp_cdds[i]->total_cps,
                total_cps,
                ((comp_cdds[i]->total_cps / (float)total_cps) * 100),
                comp_cdds[i]->timesteps );
      }
      fprintf( ofile, "\n\n" );

      /* Deallocate the spacing string */
      free_safe( str, (longest_name_len + 1) );

    }

    /* Close the file if it was opened via fopen */
    if( rank_file != NULL ) {
      rv = fclose( ofile );
      assert( rv == 0 );
    }

  } else {

    rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open ranking file \"%s\" for writing", rank_file );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Performs merge command functionality.
*/
void command_rank(
  int          argc,      /*!< Number of arguments in command-line to parse */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< List of arguments from command-line to parse */
) { PROFILE(COMMAND_RANK);

  int            i, j;
  unsigned int   rv;
  comp_cdd_cov** comp_cdds    = NULL;
  unsigned int   comp_cdd_num = 0;
  bool           error        = FALSE;

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, HEADER, __FILE__, __LINE__ );

  Try {

    unsigned int rv;
    bool         first  = TRUE;
    str_link*    strl;
    timer*       atimer = NULL;

    /* Parse score command-line */
    if( !rank_parse_args( argc, last_arg, argv ) ) {

      /* Make sure that all coverage points are accumulated */
      report_line        = TRUE;
      report_toggle      = TRUE;
      report_combination = TRUE;
      report_fsm         = TRUE;
      report_assertion   = TRUE;
      report_memory      = TRUE;
      allow_multi_expr   = FALSE;

      /* Start timer */
      if( rank_verbose ) {
        timer_clear( &atimer );
        timer_start( &atimer );
      }

      /* Read in databases to merge */
      strl = rank_in_head;
      while( strl != NULL ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Reading CDD file \"%s\"", strl->str );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, NORMAL, __FILE__, __LINE__ );
        rv = fflush( stdout );
        assert( rv == 0 );
        rank_read_cdd( strl->str, (strl->suppl == 1), first, &comp_cdds, &comp_cdd_num );
        first = FALSE;
        strl  = strl->next;
      }

      if( rank_verbose ) {
        timer_stop( &atimer );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Completed reading in CDD files in %s", timer_to_string( atimer ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, NORMAL, __FILE__, __LINE__ );
        free_safe( atimer, sizeof( timer ) );
      }

      /* Peaform the ranking algorithm */
      rank_perform( comp_cdds, comp_cdd_num );

      /* Output the results */
      rank_output( comp_cdds, comp_cdd_num );

      /*@-duplicatequals -formattype -formatcode@*/
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Dynamic memory allocated:   %" FMT64 "u bytes", largest_malloc_size );
      assert( rv < USER_MSG_LENGTH );
      /*@=duplicatequals =formattype =formatcode@*/
      print_output( "", NORMAL, __FILE__, __LINE__ );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      print_output( "", NORMAL, __FILE__, __LINE__ );

    }

  } Catch_anonymous {
    error = TRUE;
  }

  /* Deallocate other allocated variables */
  str_link_delete_list( rank_in_head );

  /* Deallocate the compressed CDD coverage structures */
  for( i=0; i<comp_cdd_num; i++ ) {
    rank_dealloc_comp_cdd_cov( comp_cdds[i] );
  }
  free_safe( comp_cdds, (sizeof( comp_cdd_cov* ) * comp_cdd_num) );

  free_safe( rank_file, (strlen( rank_file ) + 1) );

  if( error ) {
    Throw 0;
  }

  PROFILE_END;

}

