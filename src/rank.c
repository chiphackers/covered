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
#include "profiler.h"
#include "rank.h"
#include "util.h"
#include "vsignal.h"


extern char           user_msg[USER_MSG_LENGTH];
extern const exp_info exp_op_info[EXP_OP_NUM];
extern db**           db_list;
extern uint64         num_timesteps;
extern bool           output_suppressed;
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
 List of CDD filenames that need to be read in.
*/
static char** rank_in = NULL;

/*!
 Number of CDD filenames in the rank_in array.
*/
static int rank_in_num = 0;

/*!
 File to be used for outputting rank information.
*/
static char* rank_file = NULL;

/*!
 Array containing the number of coverage points for each metric for all compressed CDD coverage structures.
*/
static unsigned int num_cps[CP_TYPE_NUM] = {0};

/*!
 Array containing the weights to be used for each of the CDD metric types.
*/
static unsigned int cdd_type_weight[CP_TYPE_NUM] = {10,1,2,5,3,0};
//static unsigned int cdd_type_weight[CP_TYPE_NUM] = {1,1,1,1,1,0};

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

/*!
 \return Returns a pointer to a newly allocated and initialized compressed CDD coverage structure.
*/
comp_cdd_cov* rank_create_comp_cdd_cov(
  const char* cdd_name,  /*!< Name of CDD file that this structure was created from */
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

  for( i=0; i<CP_TYPE_NUM; i++ ) {
    comp_cov->cps_index[i] = 0;
    if( num_cps[i] > 0 ) {
      comp_cov->cps[i] = (unsigned char*)calloc_safe( ((num_cps[i] >> 3) + 1), sizeof( unsigned char ) );
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
      free_safe( comp_cov->cps[i], (sizeof( unsigned char ) * ((num_cps[i] >> 3) + 1)) );
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
  printf( "Usage:  covered rank [<options>] <database_to_rank> <database_to_rank>*\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -depth <number>           Specifies the minimum number of CDD files to hit each coverage point.\n" );
  printf( "                                  The value of <number> should be a value of 1 or more.  Default is 1.\n" );
  printf( "      -names-only               If specified, outputs only the needed CDD filenames that need to be\n" );
  printf( "                                  run in the order they need to be run.  If this option is not set, a\n" );
  printf( "                                  report-style output is provided with additional information.\n" );
  printf( "      -o <filename>             Name of file to output ranking information to.  Default is stdout.\n" );
  printf( "      -weight-line <number>     Specifies a relative weighting for line coverage used to rank\n" );
  printf( "                                  non-unique coverage points.\n" );
  printf( "      -weight-toggle <number>   Specifies a relative weighting for toggle coverage used to rank\n" );
  printf( "                                  non-unique coverage points.\n" );
  printf( "      -weight-memory <number>   Specifies a relative weighting for memory coverage used to rank\n" );
  printf( "                                  non-unique coverage points.\n" );
  printf( "      -weight-comb <number>     Specifies a relative weighting for combinational logic coverage used\n" );
  printf( "                                  to rank non-unique coverage points.\n" );
  printf( "      -weight-fsm <number>      Specifies a relative weighting for FSM state/state transition coverage\n" );
  printf( "                                  used to rank non-unique coverage points.\n" );
  printf( "      -weight-assert <number>   Specifies a relative weighting for assertion coverage used to rank\n" );
  printf( "                                  non-unique coverage points.\n" );
  printf( "      -h                        Displays this help information.\n" );
  printf( "\n" );

}

/*!
 \throws anonymous Throw Throw Throw

 Parses the score argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the rank operation, an error message is displayed to the
 user.
*/
static void rank_parse_args(
  int          argc,      /*!< Number of arguments in argument list argv */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< Argument list passed to this program */
) {

  int i;  /* Loop iterator */

  i = last_arg + 1;

  while( i < argc ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      rank_usage();
      Throw 0;

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
        if( cp_depth == 0 ) {
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

    } else {

      /* The name of a file to rank */
      if( file_exists( argv[i] ) ) {

        /* Add the specified rank file to the list */
        rank_in              = (char**)realloc_safe( rank_in, (sizeof( char* ) * rank_in_num), (sizeof( char* ) * (rank_in_num + 1)) );
        rank_in[rank_in_num] = strdup_safe( argv[i] );
        rank_in_num++;

      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "CDD file (%s) does not exist", argv[i] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    }

    i++;

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
    snprintf( user_msg, USER_MSG_LENGTH, "Last read in CDD file is incompatible with previously read in CDD files.  Exiting..." );
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
  if( (sig->suppl.part.type != SSUPPL_TYPE_PARAM) &&
      (sig->suppl.part.type != SSUPPL_TYPE_ENUM)  &&
      (sig->suppl.part.type != SSUPPL_TYPE_MEM)  &&
      (sig->suppl.part.mba == 0) ) {

    unsigned int i;
    if( sig->suppl.part.excluded == 1 ) {
      for( i=0; i<sig->value->width; i++ ) {
        uint64 index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
        rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
        comp_cov->cps[CP_TYPE_TOGGLE][index >> 3] |= 0x1 << (index & 0x7);
        index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
        rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
        comp_cov->cps[CP_TYPE_TOGGLE][index >> 3] |= 0x1 << (index & 0x7);
      }
    } else {
      switch( sig->value->suppl.part.data_type ) {
        case VDATA_UL :
          for( i=0; i<sig->value->width; i++ ) {
            uint64 index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
            rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
            comp_cov->cps[CP_TYPE_TOGGLE][index >> 3] |= (sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_SIG_TOG01] >> UL_MOD(i)) << (index & 0x7);
            index = comp_cov->cps_index[CP_TYPE_TOGGLE]++;
            rank_check_index( CP_TYPE_TOGGLE, index, __LINE__ );
            comp_cov->cps[CP_TYPE_TOGGLE][index >> 3] |= (sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_SIG_TOG10] >> UL_MOD(i)) << (index & 0x7);
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
        comp_cov->cps[CP_TYPE_MEM][index >> 3] |= 0x1 << (index & 0x7);
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][index >> 3] |= 0x1 << (index & 0x7);
      } else {
        unsigned int wr = 0;
        unsigned int rd = 0;
        uint64       index;
        vector_mem_rw_count( sig->value, (int)i, (int)((i + pwidth) - 1), &wr, &rd );
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][index >> 3] |= ((wr > 0) ? 1 : 0) << (index & 0x7);
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][index >> 3] |= ((rd > 0) ? 1 : 0) << (index & 0x7);
      }
    }

    /* Calculate toggle coverage information for the memory */
    if( sig->suppl.part.excluded == 1 ) {
      for( i=0; i<sig->value->width; i++ ) {
        uint64 index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][index >> 3] |= 0x1 << (index & 0x7);
        index = comp_cov->cps_index[CP_TYPE_MEM]++;
        rank_check_index( CP_TYPE_MEM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_MEM][index >> 3] |= 0x1 << (index & 0x7);
      }
    } else {
      switch( sig->value->suppl.part.data_type ) {
        case VDATA_UL :
          for( i=0; i<sig->value->width; i++ ) {  
            uint64 index = comp_cov->cps_index[CP_TYPE_MEM]++;
            rank_check_index( CP_TYPE_MEM, index, __LINE__ );
            comp_cov->cps[CP_TYPE_MEM][index >> 3] |= (sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_MEM_TOG01] >> UL_MOD(i)) << (index & 0x7);
            index = comp_cov->cps_index[CP_TYPE_MEM]++;
            rank_check_index( CP_TYPE_MEM, index, __LINE__ );
            comp_cov->cps[CP_TYPE_MEM][index >> 3] |= (sig->value->value.ul[UL_DIV(i)][VTYPE_INDEX_MEM_TOG10] >> UL_MOD(i)) << (index & 0x7);
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
      
      if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ||
          ((exp->op != EXP_OP_AND) &&
           (exp->op != EXP_OP_LAND) &&
           (exp->op != EXP_OP_OR)   &&
           (exp->op != EXP_OP_LOR)) ) {
  
        /* Calculate current expression combination coverage */
        if( !expression_is_static_only( exp ) ) {
    
          uint64 index;

          if( EXPR_IS_COMB( exp ) == 1 ) {
            if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= ESUPPL_WAS_FALSE( exp->left->suppl ) << (index & 0x7);
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= ESUPPL_WAS_FALSE( exp->right->suppl ) << (index & 0x7);
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= exp->suppl.part.eval_11 << (index & 0x7);
            } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= ESUPPL_WAS_TRUE( exp->left->suppl ) << (index & 0x7);
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= ESUPPL_WAS_TRUE( exp->right->suppl ) << (index & 0x7);
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= exp->suppl.part.eval_00 << (index & 0x7);
            } else {
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= exp->suppl.part.eval_00 << (index & 0x7);
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= exp->suppl.part.eval_01 << (index & 0x7);
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= exp->suppl.part.eval_10 << (index & 0x7);
              index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
              rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
              comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= exp->suppl.part.eval_11 << (index & 0x7);
            }
          } else if( EXPR_IS_EVENT( exp ) == 1 ) {
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= ESUPPL_WAS_TRUE( exp->suppl ) << (index & 0x7);
          } else {
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= ESUPPL_WAS_TRUE( exp->suppl ) << (index & 0x7);
            index = comp_cov->cps_index[CP_TYPE_LOGIC]++;
            rank_check_index( CP_TYPE_LOGIC, index, __LINE__ );
            comp_cov->cps[CP_TYPE_LOGIC][index>>3] |= ESUPPL_WAS_FALSE( exp->suppl ) << (index & 0x7);
          }
  
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
      comp_cov->cps[CP_TYPE_LINE][index >> 3] |= 0x1 << (index & 0x7);
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
          comp_cov->cps[CP_TYPE_FSM][index >> 3] |= 0x1 << (index & 0x7);
        }
        index = comp_cov->cps_index[CP_TYPE_FSM]++;
        rank_check_index( CP_TYPE_FSM, index, __LINE__ );
        comp_cov->cps[CP_TYPE_FSM][index >> 3] |= 0x1 << (index & 0x7);
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
            funit_inst*  inst,              /*!< Pointer to instance tree to calculate num_cps array */
  /*@out@*/ unsigned int nums[CP_TYPE_NUM]  /*!< Array of coverage point numbers to populate */
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

  sig_link*   sigl;   /* Pointer to signal link */
  fsm_link*   fsml;   /* Pointer to FSM link */
  funit_inst* child;  /* Pointer to current child instance */

  /* Gather coverage information from expressions */
  if( !funit_is_unnamed( inst->funit ) ) {
    func_iter  fi;
    statement* stmt;

    /* First, clear the comb_cntd bits in all of the expressions */
    func_iter_init( &fi, inst->funit );
    stmt = func_iter_get_next_statement( &fi );
    while( stmt != NULL ) {
      combination_reset_counted_expr_tree( stmt->exp );
      stmt = func_iter_get_next_statement( &fi );
    }
    func_iter_dealloc( &fi );

    /* Then populate the comp_cov structure, accordingly */
    func_iter_init( &fi, inst->funit );
    stmt = func_iter_get_next_statement( &fi );
    while( stmt != NULL ) {
      rank_gather_expression_cov( stmt->exp, stmt->suppl.part.excluded, comp_cov );
      stmt = func_iter_get_next_statement( &fi );
    }
    func_iter_dealloc( &fi );
  }

  /* Gather coverage information from signals */
  sigl = inst->funit->sig_head;
  while( sigl != NULL ) {
    rank_gather_signal_cov( sigl->sig, comp_cov );
    sigl = sigl->next;
  }

  /* Gather coverage information from FSMs */
  fsml = inst->funit->fsm_head;
  while( fsml != NULL ) {
    rank_gather_fsm_cov( fsml->table->table, comp_cov );
    fsml = fsml->next;
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
            bool            first,        /*!< Set to TRUE if this if the first CDD being read */
  /*@out@*/ comp_cdd_cov*** comp_cdds,    /*!< Pointer to compressed CDD array */
  /*@out@*/ unsigned int*   comp_cdd_num  /*!< Number of compressed CDD structures in comp_cdds array */
) { PROFILE(RANK_READ_CDD);

  comp_cdd_cov* comp_cov;

  Try {

    inst_link*   instl;
    unsigned int tmp_nums[CP_TYPE_NUM] = {0};

    /* Read in database */
    db_read( cdd_name, READ_MODE_REPORT_NO_MERGE );
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
    comp_cov = rank_create_comp_cdd_cov( cdd_name, num_timesteps );

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

  /* Output status indicator, if necessary */
  if( !output_suppressed || debug_mode ) {
    while( ((unsigned int)(((next_cdd + 1) / (float)comp_cdd_num) * 100) - (dots_output * 10)) >= 10 ) { 
      printf( "." );
      fflush( stdout );
      dots_output++;
    }
  }

  /* Move the most unique CDD to the next position */
  comp_cdd_cov* tmp       = comp_cdds[next_cdd];
  comp_cdds[next_cdd]     = comp_cdds[selected_cdd];
  comp_cdds[selected_cdd] = tmp;

  /* Zero out uniqueness value */
  comp_cdds[next_cdd]->unique_cps = 0;

  /* Subtract all of the set coverage points from the merged value */
  for( i=0; i<CP_TYPE_NUM; i++ ) {
    for( j=0; j<num_cps[i]; j++ ) {
      if( comp_cdds[next_cdd]->cps[i][j>>3] & (0x1 << (j & 0x7)) ) {
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
      merged_index++;
    }
  }

  if( !output_suppressed || debug_mode ) {
    if( (next_cdd + 1) == comp_cdd_num ) {
      if( dots_output < 10 ) {
        printf( "." );
      }
      printf( "\n" );
      fflush( stdout );
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
            uint64         merged_num,       /*!< Number of elements in merged array */
            unsigned int   next_cdd          /*!< Next index in comp_cdds array to set */
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
	  if( comp_cdds[i]->cps[j][k>>3] & (0x1 << (k & 0x7)) ) {
            total++;
            if( ranked_merged[x] < cp_depth ) {
              unique_found = TRUE;
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

#ifdef OBSOLETE
  /* First, perform the sort */
  for( i=0; i<comp_cdd_num; i++ ) {
    best = i;
    for( j=(i+1); j<comp_cdd_num; j++ ) {
      if( (comp_cdds[best]->total_cps / (float)comp_cdds[best]->timesteps) < (comp_cdds[j]->total_cps / (float)comp_cdds[j]->timesteps) ) {
        best = j;
      }
    }
    tmp             = comp_cdds[i];
    comp_cdds[i]    = comp_cdds[best];
    comp_cdds[best] = tmp;
  }

  /* Recalcuate uniqueness information */
  for( x=0; x<num_ranked; x++ ) {
    ranked_merged[x] = 0;
  }
  for( i=0; i<comp_cdd_num; i++ ) {
    x = 0;
    comp_cdds[i]->unique_cps = 0;
    for( j=0; j<CP_TYPE_NUM; j++ ) {
      for( k=0; k<num_cps[j]; k++ ) {
        if( comp_cdds[i]->cps[j][k>>3] & (0x1 << (k & 0x7)) ) {
          if( ranked_merged[x] == 0 ) {
            comp_cdds[i]->unique_cps++;
          }
          ranked_merged[x]++;
        }
        x++;
      }
    }
  }
#else
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
          if( comp_cdds[j]->cps[k][l>>3] & (0x1 << (l & 0x7)) ) {
            if( ranked_merged[x] == 0 ) {
              comp_cdds[j]->unique_cps++;
            }
          }
          x++;
        }
      }
      if( (comp_cdds[best]->unique_cps < comp_cdds[j]->unique_cps) ||
          (comp_cdds[best]->unique_cps == comp_cdds[j]->unique_cps) && (comp_cdds[best]->timesteps < comp_cdds[j]->timesteps) ) {
        best = j;
      }
    }
    tmp             = comp_cdds[i];
    comp_cdds[i]    = comp_cdds[best];
    comp_cdds[best] = tmp;
    x = 0;
    for( j=0; j<CP_TYPE_NUM; j++ ) {
      for( k=0; k<num_cps[j]; k++ ) {
        if( comp_cdds[i]->cps[j][k>>3] & (0x1 << (k & 0x7)) ) {
          ranked_merged[x]++;
        }
        x++;
      }
    }
  }
#endif

  PROFILE_END;

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
  unsigned int next_cdd     = 0;
  unsigned int most_unique;

  if( !output_suppressed || debug_mode ) {
    printf( "Ranking CDD files " );
    fflush( stdout );
  }

  /* Calculate the total number of needed merged entries to store accumulated information */
  for( i=0; i<CP_TYPE_NUM; i++ ) {
    total += num_cps[i];
  }
  assert( total > 0 );

  /* Allocate merged array */
  ranked_merged   = (uint16*)calloc_safe( total, sizeof( uint16 ) );
  unranked_merged = (uint16*)malloc_safe_nolimit( sizeof( uint16 ) * total );

  /* Step 1 - Initialize merged results array, calculate uniqueness and total values of each compressed CDD coverage structure */
  for( i=0; i<CP_TYPE_NUM; i++ ) {
    for( j=0; j<num_cps[i]; j++ ) {
      uint16       bit_total = 0;
      unsigned int set_cdd;
      for( k=0; k<comp_cdd_num; k++ ) {
        if( comp_cdds[k]->cps[i][j>>3] & (0x1 << (j & 0x7)) ) {
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

  /* Step 2 - Start with the most unique CDDs */
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

  /* Step 3 - Select coverage based on user-specified factors */
  if( next_cdd < comp_cdd_num ) {
    rank_perform_weighted_selection( comp_cdds, comp_cdd_num, ranked_merged, unranked_merged, total, next_cdd );
  }

  /* Step 4 - Re-sort the list using a greedy algorithm */
  rank_perform_greedy_sort( comp_cdds, comp_cdd_num, ranked_merged, total );

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

  FILE* ofile;

  if( rank_file == NULL ) {
    print_output( "Generating report output to standard output...", NORMAL, __FILE__, __LINE__ );
  } else {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Generating report file \"%s\"...", rank_file );
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

      /* Header information output */
      fprintf( ofile, "                                           ::::::::::::::::::::::::::::::::::::::::::::::::::::\n" );
      fprintf( ofile, "                                           ::                                                ::\n" );
      fprintf( ofile, "                                           ::     Covered -- Simulation Ranked Run Order     ::\n" );
      fprintf( ofile, "                                           ::                                                ::\n" );
      fprintf( ofile, "                                           ::::::::::::::::::::::::::::::::::::::::::::::::::::\n\n\n" );
      fprintf( ofile, "\n" );

      /* Calculate and display reduction status */
      i = 0;
      while( (i<comp_cdd_num) && (comp_cdds[i]->unique_cps > 0) ) i++;
      if( i == comp_cdd_num ) {
        fprintf( ofile, "No reduction occurred\n" );
      } else {
        fprintf( ofile, "Reduced %u CDD files down to %u needed to maintain coverage (%3.0f%% reduction)\n", comp_cdd_num, i, (((comp_cdd_num - i) / (float)comp_cdd_num) * 100) );
      }
      fprintf( ofile, "\n" );
      fprintf( ofile, "-----------+-------------------------------------------+----------------------------------------------------------------------------------------------\n" );
      fprintf( ofile, "           |                ACCUMULATIVE               |                                               CDD\n" );
      fprintf( ofile, "Simulation |-------------------------------------------+----------------------------------------------------------------------------------------------\n" );
      fprintf( ofile, "Order      |        Hit /      Total     %%   Timesteps |  Name                                                      Hit /      Total     %%   Timesteps\n" );
      fprintf( ofile, "-----------+-------------------------------------------+----------------------------------------------------------------------------------------------\n" );
      fprintf( ofile, "\n" );

      for( i=0; i<comp_cdd_num; i++ ) {
        acc_timesteps  += comp_cdds[i]->timesteps; 
        acc_unique_cps += comp_cdds[i]->unique_cps;
        if( (comp_cdds[i]->unique_cps == 0) && unique_found ) {
          fprintf( ofile, "\n---------------------------------------  The following CDD files add no additional coverage  ----------------------------------------------\n\n" );
          unique_found = FALSE;
        }
        fprintf( ofile, "%10u   %10llu   %10llu  %3.0f%%  %10llu   %-50s  %10llu   %10llu  %3.0f%%  %10llu\n",
                (i + 1),
                acc_unique_cps,
                total_cps,
                ((acc_unique_cps / (float)total_cps) * 100),
                acc_timesteps,
                comp_cdds[i]->cdd_name,
                comp_cdds[i]->total_cps,
                total_cps,
                ((comp_cdds[i]->total_cps / (float)total_cps) * 100),
                comp_cdds[i]->timesteps );
      }
      fprintf( ofile, "\n\n" );

    }

    /* Close the file if it was opened via fopen */
    if( rank_file != NULL ) {
      rv = fclose( ofile );
      assert( rv == 0 );
    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open ranking file \"%s\" for writing", rank_file );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param argc      Number of arguments in command-line to parse.
 \param last_arg  Index of last parsed argument from list.
 \param argv      List of arguments from command-line to parse.

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

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, NORMAL, __FILE__, __LINE__ );

  Try {

    unsigned int rv;

    /* Parse score command-line */
    rank_parse_args( argc, last_arg, argv );

    /* Make sure that all coverage points are accumulated */
    report_line        = TRUE;
    report_toggle      = TRUE;
    report_combination = TRUE;
    report_fsm         = TRUE;
    report_assertion   = TRUE;
    report_memory      = TRUE;
    allow_multi_expr   = FALSE;

    /* Read in databases to merge */
    for( i=0; i<rank_in_num; i++ ) {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Reading CDD file \"%s\"", rank_in[i] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      rank_read_cdd( rank_in[i], (i == 0), &comp_cdds, &comp_cdd_num );
    }

    /* Peaform the ranking algorithm */
    rank_perform( comp_cdds, comp_cdd_num );

    /* Output the results */
    rank_output( comp_cdds, comp_cdd_num );

    /*@-duplicatequals -formattype@*/
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Dynamic memory allocated:   %llu bytes", largest_malloc_size );
    assert( rv < USER_MSG_LENGTH );
    /*@=duplicatequals =formattype@*/
    print_output( "", NORMAL, __FILE__, __LINE__ );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    print_output( "", NORMAL, __FILE__, __LINE__ );

  } Catch_anonymous {}

  /* Deallocate other allocated variables */
  for( i=0; i<rank_in_num; i++ ) {
    free_safe( rank_in[i], (strlen( rank_in[i] ) + 1) );
  }
  free_safe( rank_in, (sizeof( char* ) * rank_in_num) );

  /* Deallocate the compressed CDD coverage structures */
  for( i=0; i<comp_cdd_num; i++ ) {
    rank_dealloc_comp_cdd_cov( comp_cdds[i] );
  }
  free_safe( comp_cdds, (sizeof( comp_cdd_cov* ) * comp_cdd_num) );

  free_safe( rank_file, (strlen( rank_file ) + 1) );

  PROFILE_END;

}

/*
 $Log$
 Revision 1.1.2.11  2008/07/21 05:53:25  phase1geo
 Fixing rank command issues (bugs 2018194 and 2021340).  Also added a
 note in the ranking report file that shows the amount of CDD file reduction
 that was calculated.

 Revision 1.1.2.10  2008/07/14 22:15:04  phase1geo
 Removing multi-expressions from ranking coverage point consideration.  Treating
 these as individual expressions.

 Revision 1.1.2.9  2008/07/14 18:43:43  phase1geo
 Fixing issue with toggle index getting incremented when calculating memory toggle coverage.

 Revision 1.1.2.8  2008/07/11 18:47:30  phase1geo
 Attempting to fix bug 2016187.

 Revision 1.1.2.7  2008/07/02 23:10:38  phase1geo
 Checking in work on rank function and addition of -m option to score
 function.  Added new diagnostics to verify beginning functionality.
 Checkpointing.

 Revision 1.1.2.6  2008/07/02 13:51:24  phase1geo
 Fixing bug in read mode chosen for ranking.  Should have been a report-no-merge
 rather than a merge-no-merge flavor.

 Revision 1.1.2.5  2008/07/02 04:40:18  phase1geo
 Adding merge5* diagnostics to verify rank function (this is not complete yet).  The
 rank function is a bit broken at this point.  Checkpointing.

 Revision 1.1.2.4  2008/07/01 23:08:58  phase1geo
 Initial working version of rank command.  Ranking algorithm needs some more
 testing at this point.  Checkpointing.

 Revision 1.1.2.3  2008/07/01 06:17:22  phase1geo
 More updates to rank command.  Updating IV/Cver regression for these changes (full
 regression not passing at this point).  Checkpointing.

 Revision 1.1.2.2  2008/06/30 23:02:42  phase1geo
 More work on the rank command.  Checkpointing.

 Revision 1.1.2.1  2008/06/30 13:14:22  phase1geo
 Starting to work on new 'rank' command.  Checkpointing.

*/

