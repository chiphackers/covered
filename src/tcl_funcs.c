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
 \file     tcl_funcs.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/26/2004
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_TCLTK
#include <tcl.h>
#include <tk.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "assertion.h"
#include "comb.h"
#include "defines.h"
#include "exclude.h"
#include "expr.h"
#include "fsm.h"
#include "func_unit.h"
#include "instance.h"
#include "line.h"
#include "link.h"
#include "memory.h"
#include "ovl.h"
#include "race.h"
#include "report.h"
#include "score.h"
#include "search.h"
#include "tcl_funcs.h"
#include "toggle.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern funit_link*  funit_head;
extern char         user_msg[USER_MSG_LENGTH];
extern const char*  race_msgs[RACE_TYPE_NUM];
extern char         score_run_path[4096];
extern str_link*    score_args_head;
extern str_link*    score_args_tail;
extern void         reset_pplexer( const char* filename, FILE* out );
extern int          PPVLlex( void );
extern str_link*    merge_in_head;
extern str_link*    merge_in_tail;
extern int          merge_in_num;
extern char*        output_file;
extern int          report_comb_depth; 
extern bool         report_line;
extern bool         report_toggle;
extern bool         report_memory;
extern bool         report_combination;
extern bool         report_fsm;
extern bool         report_assertion;
extern bool         report_race;
extern bool         report_instance;
extern isuppl       info_suppl;
extern int          merge_er_value;


/*!
 List of functional units that will be looked up for coverage.
*/
static func_unit** gui_funit_list = NULL;

/*!
 Current index to store next functional unit pointer in gui_funit_list.
*/
static int gui_funit_index = 0;

/*!
 List of functional unit instances that will be looked up for coverage purposes.
*/
static funit_inst** gui_inst_list = NULL;

/*!
 Current index to store next function unit instance pointer in gui_inst_list.
*/
static int gui_inst_index = 0;


/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Retrieves all of the race condition messages for all possible race conditions and stores them into
 the "race_msgs" global array.
*/
int tcl_func_get_race_reason_msgs(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_RACE_REASON_MSGS);

  int retval = TCL_OK;  /* Return value of this function */
  int i;                /* Loop iterator */

  for( i=0; i<RACE_TYPE_NUM; i++ ) {
    strcpy( user_msg, race_msgs[i] );
    Tcl_SetVar( tcl, "race_msgs", user_msg, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "funit_names" and "funit_types" with all of the functional units from the
 design.
*/
int tcl_func_get_funit_list(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_FUNIT_LIST);

  int  retval = TCL_OK;  /* Return value of this function  */
  char str[30];          /* Temporary string */
  int  i;                /* Loop iterator */

  /* Create the functional unit list */
  for( i=0; i<gui_funit_index; i++ ) {
    snprintf( str, 30, "%d 0", i );
    Tcl_AppendElement( tcl, str );
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "inst_list", "funit_names", and "funit_types" with all of the instances
 from the design.
*/
int tcl_func_get_instances(
  Tcl_Interp* tcl,  /*!< Pointer to the Tcl interpreter */
  funit_inst* root  /*!< Pointer to current root instance to output */
) { PROFILE(TCL_FUNC_GET_INSTANCES);

  funit_inst* curr;     /* Pointer to current functional unit instance */

  if( (root->funit != NULL) && !funit_is_unnamed( root->funit ) &&
      ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( funit_get_curr_module_safe( root->funit ) )) ) {
    gui_inst_list = (funit_inst**)realloc_safe( gui_inst_list, (sizeof( funit_inst* ) * gui_inst_index), (sizeof( funit_inst* ) * (gui_inst_index + 1)) );
    gui_inst_list[gui_inst_index] = root;
    gui_inst_index++;
  }

  curr = root->child_head;
  while( curr != NULL ) {
    tcl_func_get_instances( tcl, curr );
    curr = curr->next;
  }

  return( TCL_OK );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the block instance list in the format of {instance_index 1}
*/
int tcl_func_get_instance_list(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_INSTANCE_LIST);

  int        retval = TCL_OK;  /* Return value for this function */
  inst_link* instl;            /* Pointer to current instance link */
  int        i;                /* Loop iterator */
  char       str[30];          /* Temporary string */

  /* Create the functional unit list */
  for( i=0; i<gui_inst_index; i++ ) {
    snprintf( str, 30, "%d 1", i );
    Tcl_AppendElement( tcl, str );
  }

  return( retval );

}

/*!
 \return Returns TRUE if the specified argument is referencing a functional unit; otherwise, returns FALSE to indicate
         that the specified argument is referencing a functional unit instance.
*/
static bool tcl_func_is_funit(
  Tcl_Interp* tcl,  /*!< Pointer to Tcl interpreter */
  const char* arg   /*!< Pointer to argument that contains functional unit/instance information */
) { PROFILE(TCL_FUNC_IS_FUNIT);

  bool         retval = FALSE;  /* Return value for this function */
  int          argc;
  const char** argv;

  /* We need to split the given argument to get the functional unit/instance index and type */
  if( Tcl_SplitList( tcl, arg, &argc, &argv ) == TCL_OK ) {

    if( argc == 2 ) {
      retval = (atoi( argv[1] ) == 0);
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the given functional unit associated with the given index (if it is within the acceptable range);
         otherwise, returns NULL to indicate that an error occurred.
*/
static func_unit* tcl_func_get_funit(
  Tcl_Interp* tcl,  /*!< Pointer to Tcl interpreter */
  const char* arg   /*!< Pointer to argument that contains functional unit/instance information */
) { PROFILE(TCL_FUNC_GET_FUNIT);

  func_unit*   funit = NULL;  /* Pointer to found functional unit */
  int          argc;
  const char** argv;

  /* We need to split the given argument to get the functional unit/instance index and type */
  if( Tcl_SplitList( tcl, arg, &argc, &argv ) == TCL_OK ) {

    if( argc == 2 ) {

      int index = atoi( argv[0] );
      int type  = atoi( argv[1] );

      if( type == 0 ) {
        if( (index >= 0) && (index < gui_funit_index) ) {
          funit = gui_funit_list[index];
        }
      } else {
        if( (index >= 0) && (index < gui_inst_index) ) {
          funit = gui_inst_list[index]->funit;
        }
      }

    }

    /* Deallocate the given memory */
    Tcl_Free( (char*)argv );

  }

  PROFILE_END;

  return( funit );

}

/*!
 \return Returns a pointer to the given functional unit associated with the given index (if it is within the acceptable range);
         otherwise, returns NULL to indicate that an error occurred.
*/
static funit_inst* tcl_func_get_inst(
  Tcl_Interp* tcl,  /*!< Pointer to Tcl interpreter */
  const char* arg   /*!< Pointer to argument that contains functional unit/instance information */
) { PROFILE(TCL_FUNC_GET_INST);

  funit_inst*  inst = NULL;
  int          argc;
  const char** argv;

  /* We need to split the given argument to get the functional unit/instance index and type */
  if( Tcl_SplitList( tcl, arg, &argc, &argv ) == TCL_OK ) {

    if( argc == 2 ) {

      int index = atoi( argv[0] );
      int type  = atoi( argv[1] );

      if( type == 1 ) {
        if( (index >= 0) && (index < gui_inst_index) ) {
          inst = gui_inst_list[index];
        }
      }

    }

    /* Deallocate the given memory */
    Tcl_Free( (char*)argv );

  }

  PROFILE_END;

  return( inst );

}

/*! 
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Gets the filename for the specified functional unit.
*/
int tcl_func_get_funit_name(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_FUNIT_NAME);

  int        retval = TCL_OK;  /* Return value for this function */
  func_unit* funit;

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
    Tcl_SetResult( tcl, funit->name, TCL_STATIC );
  } else {
    strcpy( user_msg, "Internal Error:  Unable to find specified functional unit" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Gets the filename for the specified functional unit.
*/
int tcl_func_get_filename(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_FILENAME);

  int        retval = TCL_OK;  /* Return value for this function */
  func_unit* funit;

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
    Tcl_SetResult( tcl, funit->orig_fname, TCL_STATIC );
  } else {
    strcpy( user_msg, "Internal Error:  Unable to find specified functional unit" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Gets the instance scope for the specified functional unit.
*/
int tcl_func_get_inst_scope(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_INST_SCOPE);

  int          retval = TCL_OK;  /* Return value for this function */
  func_unit*   funit;
  int          targc;
  const char** targv;

  if( Tcl_SplitList( tcl, argv[1], &targc, &targv ) == TCL_OK ) {
    if( argc == 2 ) {
      char scope[4096];
  
      /* Generate the name of this child */
      scope[0] = '\0';
      instance_gen_scope( scope, gui_inst_list[ atoi( targv[0] ) ], FALSE );
      Tcl_SetResult( tcl, scope, TCL_VOLATILE );
    }
    Tcl_Free( (char*)targv );
  } else {
    strcpy( user_msg, "Internal Error:  Unable to find specified functional unit instance" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the starting and ending line numbers of the specified functional unit within its file.
*/
int tcl_func_get_funit_start_and_end(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_FUNIT_START_AND_END);

  int        retval = TCL_OK;  /* Return value for this function */
  char       linenum[50];      /* Temporary string container */
  func_unit* funit;            /* Pointer to functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
    snprintf( linenum, 50, "%d %d", funit->start_line, funit->end_line );
    Tcl_SetResult( tcl, linenum, TCL_VOLATILE );
  } else {
    strcpy( user_msg, "Internal Error:  Unable to find start and end lines for functional unit" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the line numbers and exclusion values of all lines that were found to be uncovered
 during simulation.
*/
int tcl_func_collect_uncovered_lines(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_UNCOVERED_LINES);

  int        retval = TCL_OK;  /* Return value for this function */
  int*       lines;            /* Array of line numbers that were found to be uncovered during simulation */
  int*       excludes;         /* Array of exclude values */
  char**     reasons;          /* Array of exclude reasons */
  int        line_cnt;         /* Number of elements in the lines and excludes arrays */
  int        line_size;        /* Number of elements allocated in lines and excludes arrays */
  int        i;                /* Loop iterator */
  char       str[50];          /* Temporary string container */
  func_unit* funit;            /* Pointer to found functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    line_collect( funit, 0, &lines, &excludes, &reasons, &line_cnt, &line_size );

    for( i=0; i<line_cnt; i++ ) {
      snprintf( str, 50, "%d %d {%s}", lines[i], excludes[i], ((reasons[i] != NULL) ? reasons[i] : "") );
      Tcl_AppendElement( tcl, str );
      free_safe( reasons[i], (strlen( reasons[i] ) + 1) );
    }

    free_safe( lines,    (sizeof( int ) * line_size) );
    free_safe( excludes, (sizeof( int ) * line_size) );
    free_safe( reasons,  (sizeof( char* ) * line_size) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variable "covered_lines" with the line numbers of all lines that were found to be covered
 during simulation.
*/
int tcl_func_collect_covered_lines(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_COVERED_LINES);

  int        retval  = TCL_OK;  /* Return value for this function */
  int*       lines;             /* Array of line numbers that were covered during simulation */
  int*       excludes;          /* Array of exclusion values */
  char**     reasons;           /* Array of exclusion reasons */
  int        line_cnt;          /* Number of elements in the lines and excludes arrays */
  int        line_size;         /* Number of elements allocated in the lines and excludes arrays */
  int        i;                 /* Loop iterator */
  char       str[50];           /* Temporary string container */
  func_unit* funit;             /* Pointer to found functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    line_collect( funit, 1, &lines, &excludes, &reasons, &line_cnt, &line_size );

    for( i=0; i<line_cnt; i++ ) {
      snprintf( str, 50, "%d %d {%s}", lines[i], excludes[i], ((reasons[i] != NULL) ? reasons[i] : "") );
      Tcl_AppendElement( tcl, str );
      free_safe( reasons[i], (strlen( reasons[i] ) + 1) );
    }

    free_safe( lines,    (sizeof( int ) * line_size) );
    free_safe( excludes, (sizeof( int ) * line_size) );
    free_safe( reasons,  (sizeof( char* ) * line_size) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the race condition information (lines reason) for the specified functional unit.
*/
int tcl_func_collect_race_lines(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_RACE_LINES);

  int        retval = TCL_OK;  /* Return value for this function */
  int        start_line;       /* Starting line of specified functional unit */
  int*       slines;           /* Starting line numbers of statement blocks containing race condition(s) */
  int*       elines;           /* Ending line numbers of statement blocks containing race conditions(s) */
  int*       reasons;          /* Reason for race condition for a specified statement block */
  int        line_cnt;         /* Number of valid entries in the slines, elines and reasons arrays */
  int        i      = 0;       /* Loop iterator */
  char       line[70];         /* Temporary string containing line information */
  func_unit* funit;            /* Pointer to found functional unit */

  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    race_collect_lines( funit, &slines, &elines, &reasons, &line_cnt );

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line, 70, "{%d.0 %d.end} %d", (slines[i] - (start_line - 1)), (elines[i] - (start_line - 1)), reasons[i] );
      Tcl_AppendElement( tcl, line );
    }

    free_safe( slines,  (sizeof( int ) * line_cnt) );
    free_safe( elines,  (sizeof( int ) * line_cnt) );
    free_safe( reasons, (sizeof( int ) * line_cnt) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the names of all signals of the given functional unit that did not achieve 100% toggle coverage.
*/
int tcl_func_collect_uncovered_toggles(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_UNCOVERED_TOGGLES);

  int        retval   = TCL_OK;  /* Return value for this function */
  char       tmp[120];           /* Temporary string */
  int        start_line;         /* Starting line number */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this command */
  start_line = atoi( argv[2] );

  /* Find all signals that did not achieve 100% coverage */
  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    unsigned int i;
    vsignal**    sigs            = NULL;
    unsigned int sig_size        = 0;
    unsigned int sig_no_rm_index = 1;

    toggle_collect( funit, 0, &sigs, &sig_size, &sig_no_rm_index );

    for( i=0; i<sig_size; i++ ) {
      snprintf( tmp, 120, "{%d.%d %d.%d} %d",
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + 14),
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + ((int)strlen( sigs[i]->name ) - 1) + 15),
                sigs[i]->suppl.part.excluded );
      Tcl_AppendElement( tcl, tmp );
    }

    /* Deallocate signal list (without destroying signals) */
    sig_link_delete_list( sigs, sig_size, sig_no_rm_index, FALSE );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variable "covered_toggles" with the names of all signals of the given functional unit
 that achieved 100% toggle coverage.
*/
int tcl_func_collect_covered_toggles(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_COVERED_TOGGLES);

  int        retval   = TCL_OK;  /* Return value for this function */
  char       tmp[85];            /* Temporary string */
  int        start_line;         /* Starting line number of this functional unit */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this function call */
  start_line = atoi( argv[2] );

  /* Get the toggle information for all covered signals */
  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    vsignal**    sigs            = NULL;
    unsigned int sig_size        = 0;
    unsigned int sig_no_rm_index = 1;
    unsigned int i;

    toggle_collect( funit, 1, &sigs, &sig_size, &sig_no_rm_index );

    for( i=0; i<sig_size; i++ ) {
      snprintf( tmp, 85, "%d.%d %d.%d",
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + 14),
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + ((int)strlen( sigs[i]->name ) - 1) + 15) );
      Tcl_AppendElement( tcl, tmp );
    }

    /* Deallocate list of signals (without deallocating the signals themselves) */
    sig_link_delete_list( sigs, sig_size, sig_no_rm_index, FALSE );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the starting and ending indices and exclusion information of all signals of the given functional unit that achieved less than 100% memory coverage.
*/
int tcl_func_collect_uncovered_memories(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_UNCOVERED_MEMORIES);

  int        retval   = TCL_OK;  /* Return value for this function */
  char       tmp[120];           /* Temporary string */
  int        start_line;         /* Starting line number of this functional unit */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this function call */
  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    vsignal** sigs               = NULL;
    unsigned int sig_size        = 0;
    unsigned int sig_no_rm_index = 1;
    unsigned int i;

    /* Get the memory information for all uncovered signals */
    memory_collect( funit, 0, &sigs, &sig_size, &sig_no_rm_index );

    /* Populate uncovered_memories array */
    for( i=0; i<sig_size; i++ ) {
      snprintf( tmp, 120, "%d.%d %d.%d %d",
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + 14),
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + ((int)strlen( sigs[i]->name ) - 1) + 15),
                sigs[i]->suppl.part.excluded );
      Tcl_AppendElement( tcl, tmp );
    }

    /* Deallocate list of signals (without deallocating the signals themselves) */
    sig_link_delete_list( sigs, sig_size, sig_no_rm_index, FALSE );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the starting and ending indices and exclusion information of all signals of the given functional unit that achieved less than 100% memory coverage.
*/
int tcl_func_collect_covered_memories(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_COVERED_MEMORIES);

  int        retval   = TCL_OK;  /* Return value for this function */
  char       tmp[120];           /* Temporary string */
  int        start_line;         /* Starting line number of this functional unit */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this function call */
  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    vsignal**    sigs            = NULL;
    unsigned int sig_size        = 0;
    unsigned int sig_no_rm_index = 1;
    unsigned int i;

    /* Get the memory information for all covered signals */
    memory_collect( funit, 1, &sigs, &sig_size, &sig_no_rm_index );

    /* Populate covered_memories array */
    for( i=0; i<sig_size; i++ ) {
      snprintf( tmp, 120, "%d.%d %d.%d %d",
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + 14),
                (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + ((int)strlen( sigs[i]->name ) - 1) + 15), 
                sigs[i]->suppl.part.excluded );
      Tcl_AppendElement( tcl, tmp );
    }

    /* Deallocate list of signals (without deallocating the signals themselves) */
    sig_link_delete_list( sigs, sig_size, sig_no_rm_index, FALSE );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the verbose coverage information for the specified signal in the specified functional unit in a
 list format {msb, lsb, tog01, tog10, excluded, reason}.
*/
int tcl_func_get_toggle_coverage(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_TOGGLE_COVERAGE);

  int        retval = TCL_OK;  /* Return value for this function */
  char*      signame;          /* Name of signal to get verbose toggle information for */
  char       tmp[20];          /* Temporary string for conversion purposes */
  func_unit* funit;            /* Pointer to found functional unit */

  signame = strdup_safe( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    int   msb;
    int   lsb;
    char* tog01;
    char* tog10;
    int   excluded;
    char* reason;
    char* str;
    int   size;

    toggle_get_coverage( funit, signame, &msb, &lsb, &tog01, &tog10, &excluded, &reason );

    size = 20 + 1 + 20 + 1 + strlen( tog01 ) + 1 + strlen( tog10 ) + 1 + 20 + 1 + ((reason != NULL) ? strlen( reason ) : 0) + 2 + 1;
    str  = Tcl_Alloc( size );

    /* Create the result */
    snprintf( str, size, "%d %d %s %s %d {%s}", msb, lsb, tog01, tog10, excluded, ((reason != NULL) ? reason : "") );
    Tcl_SetResult( tcl, str, TCL_DYNAMIC );

    /* Free up allocated memory */
    free_safe( tog01, (strlen( tog01 ) + 1) );
    free_safe( tog10, (strlen( tog10 ) + 1) );
    free_safe( reason, (strlen( reason ) + 1) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  /* Free up allocated memory */
  free_safe( signame, (strlen( signame ) + 1) );

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the verbose memory coverage information for the specified signal in the specified functional unit.
*/
int tcl_func_get_memory_coverage(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_MEMORY_COVERAGE);

  int        retval = TCL_OK;  /* Return value for this function */
  char*      signame;          /* Name of signal to get verbose toggle information for */
  char*      pdim_str;         /* String containing signal packed dimensional information */
  char*      pdim_array;       /* String containing signal packed dimensional bit information */
  char*      udim_str;         /* String containing signal unpacked dimensional information */
  char*      memory_info;      /* Memory information */
  int        excluded;         /* Specifies if signal should be excluded */
  char*      reason;           /* Exclusion reason */
  char       str[200];         /* Temporary string for conversion purposes */
  func_unit* funit;            /* Pointer to found functional unit */

  signame = strdup_safe( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    int   str_size;
    char* str;

    memory_get_coverage( funit, signame, &pdim_str, &pdim_array, &udim_str, &memory_info, &excluded, &reason );

    str_size = strlen( udim_str ) + 1 + strlen( pdim_str ) + 1 + strlen( pdim_array ) + 1 + strlen( memory_info ) + 1 + 20 + 1 + ((reason != NULL) ? strlen( reason ) : 0) + 1;
    str      = Tcl_Alloc( str_size );

    snprintf( str, str_size, "{%s} {%s} {%s} {%s} %d {%s}", udim_str, pdim_str, pdim_array, memory_info, excluded, ((reason != NULL) ? reason : "") );
    Tcl_SetResult( tcl, str, TCL_DYNAMIC );

    /* Free up allocated memory */
    free_safe( pdim_str, (strlen( pdim_str ) + 1) );
    free_safe( pdim_array, (strlen( pdim_array ) + 1) );
    free_safe( udim_str, (strlen( udim_str ) + 1) );
    free_safe( memory_info, (strlen( memory_info ) + 1) );

  } else {
    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free up allocated memory */
  free_safe( signame, (strlen( signame ) + 1) );

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the uncovered expression line/character information and exclusion property for uncovered combinational
 expressions within the given functional unit.
*/
int tcl_func_collect_uncovered_combs(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_UNCOVERED_COMBS);

  int          retval = TCL_OK;  /* Return value for this function */
  expression** exprs;            /* Array of expression pointers to uncovered expressions */
  int*         excludes;         /* Array of integers indicating exclude status */
  unsigned int exp_cnt;          /* Number of elements in the exprs array */
  unsigned int i;                /* Loop iterator */
  char         str[85];          /* Temporary string container */
  int          startline;        /* Starting line number of this module */
  expression*  last;             /* Pointer to expression in an expression tree that is on the last line */
  func_unit*   funit;            /* Pointer to found functional unit */

  startline = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) { 

    combination_collect( funit, 0, &exprs, &exp_cnt, &excludes );

    /* Load uncovered statements into Tcl */
    for( i=0; i<exp_cnt; i++ ) {
      last = expression_get_last_line_expr( exprs[i] );
      snprintf( str, 85, "%d.%d %d.%d %d %d", (exprs[i]->line - (startline - 1)), (exprs[i]->col.part.first + 14),
                                              (last->line     - (startline - 1)), (last->col.part.last + 15),
                                              exprs[i]->id, excludes[i] );
      Tcl_AppendElement( tcl, str );
    }

    /* Deallocate memory */
    free_safe( exprs, (sizeof( expression ) * exp_cnt) );
    free_safe( excludes, (sizeof( int ) * exp_cnt) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the covered expression line/character information and exclusion property for covered combinational
 expressions within the given functional unit.
*/
int tcl_func_collect_covered_combs(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_COVERED_COMBS);

  int          retval = TCL_OK;  /* Return value for this function */
  expression** exprs;            /* Array of expression pointers to covered expressions */
  int*         excludes;         /* Array of integers indicating exclude status */
  unsigned int exp_cnt;          /* Number of elements in the exprs array */
  unsigned int i;                /* Loop iterator */
  char         str[85];          /* Temporary string container */
  int          startline;        /* Starting line number of this module */
  expression*  last;             /* Pointer to expression in an expression tree that is on the last line */
  func_unit*   funit;            /* Pointer to found functional unit */

  startline = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    combination_collect( funit, 1, &exprs, &exp_cnt, &excludes );

    /* Load covered statements into Tcl */
    for( i=0; i<exp_cnt; i++ ) {
      last = expression_get_last_line_expr( exprs[i] );
      snprintf( str, 85, "%d.%d %d.%d %d %d", (exprs[i]->line - (startline - 1)), (exprs[i]->col.part.first + 14),
                                              (last->line     - (startline - 1)), (last->col.part.last + 15),
                                              exprs[i]->id, excludes[i] );
      Tcl_AppendElement( tcl, str );
    }

    /* Deallocate memory */
    free_safe( exprs, (sizeof( expression ) * exp_cnt) );
    free_safe( excludes, (sizeof( int ) * exp_cnt) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Retrieves the verbose combination expression information for a given expression, populating the "comb_code",
 "comb_uline_groups" and "comb_ulines" global variables with the code and underline information.
*/
int tcl_func_get_comb_expression(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_COMB_EXPRESSION);

  int          retval = TCL_OK;  /* Return value for this function */
  int          expr_id;          /* Expression ID of expression to find within the given functional unit */
  char**       code;             /* Array of strings containing the combinational logic code returned from the code generator */
  int*         uline_groups;     /* Array of integers representing the number of underline lines found under each line of code */
  unsigned int code_size;        /* Number of elements stored in the code array */
  char**       ulines;           /* Array of strings containing the underline lines returned from the underliner */
  unsigned int uline_size;       /* Number of elements stored in the ulines array */
  int*         excludes;         /* Array of integers containing the exclude value for a given underlined expression */
  char**       reasons;          /* Array of strings containing the exclude reason for a given underlined expression */
  unsigned int exclude_size;     /* Number of elements stored in the excludes array */
  unsigned int i;                /* Loop iterator */
  char         tmp[20];          /* Temporary string container */

  expr_id = atoi( argv[2] );

  /* Set the curr_db value appropriately for combination_get_expression */
  if( tcl_func_is_funit( tcl, argv[1] ) ) {
    curr_db = 1;
  } else {
    curr_db = 0;
  }

  combination_get_expression( expr_id, &code, &uline_groups, &code_size, &ulines, &uline_size, &excludes, &reasons, &exclude_size );

  for( i=0; i<code_size; i++ ) {
    Tcl_SetVar( tcl, "comb_code", code[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    snprintf( tmp, 20, "%d", uline_groups[i] );
    Tcl_SetVar( tcl, "comb_uline_groups", tmp, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    free_safe( code[i], (strlen( code[i] ) + 1) );
  }

  for( i=0; i<uline_size; i++ ) {
    Tcl_SetVar( tcl, "comb_ulines", ulines[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    free_safe( ulines[i], (strlen( ulines[i] ) + 1) );
  }

  for( i=0; i<exclude_size; i++ ) {
    snprintf( tmp, 20, "%d", excludes[i] );
    Tcl_SetVar( tcl, "comb_exp_excludes", tmp, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    Tcl_SetVar( tcl, "comb_exp_reasons", ((reasons[i] != NULL) ? reasons[i] : "{}"), (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
  }

  /* Free up allocated memory */
  if( code_size > 0 ) {
    free_safe( code, (sizeof( char* ) * code_size) );
    free_safe( uline_groups, (sizeof( char* ) * code_size) );
  }

  if( uline_size > 0 ) {
    free_safe( ulines, (sizeof( char* ) * uline_size) );
  }

  if( exclude_size > 0 ) {
    free_safe( excludes, (sizeof( int ) * exclude_size) );
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "comb_expr_cov" global variable with the coverage information for the specified
 subexpression with the given underline identifier.
*/
int tcl_func_get_comb_coverage(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_COMB_COVERAGE);

  int    retval = TCL_OK;  /* Return value for this function */
  int    expid;            /* Expression ID of statement containing desired subexpression */
  int    ulid;             /* Underline ID of expression to find */
  int    i;                /* Loop iterator */
  char** info;
  int    info_size;

  expid = atoi( argv[2] );
  ulid  = atoi( argv[3] );

  /* Calculate the value of curr_db */
  curr_db = tcl_func_is_funit( tcl, argv[1] ) ? 1 : 0;

  combination_get_coverage( expid, ulid, &info, &info_size );

  if( info_size > 0 ) {

    for( i=0; i<info_size; i++ ) {
      Tcl_AppendElement( tcl, info[i] );
      free_safe( info[i], (strlen( info[i] ) + 1) );
    }

    free_safe( info, (sizeof( char* ) * info_size) );

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the uncovered FSM expression line/character values for each.
*/
int tcl_func_collect_uncovered_fsms(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_UNCOVERED_FSMS);

  int        retval = TCL_OK;  /* Return value for this function */
  char       str[85];          /* Temporary string container */
  int        start_line;       /* Starting line number of this module */
  int*       expr_ids;         /* Array containing the statement IDs of all uncovered FSM signals */
  int*       excludes;         /* Array containing exclude values of all uncovered FSM signals */
  func_unit* funit;            /* Pointer to found functional unit */

  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    vsignal**    sigs            = NULL;
    unsigned int sig_size        = 0;
    unsigned int sig_no_rm_index = 1;
    unsigned int i;

    fsm_collect( funit, 0, &sigs, &sig_size, &sig_no_rm_index, &expr_ids, &excludes );

    /* Load uncovered FSMs into Tcl */
    for( i=0; i<sig_size; i++ ) {
      snprintf( str, 85, "%d.%d %d.%d %d %d", (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + 14),
                                              (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + ((int)strlen( sigs[i]->name ) - 1) + 15),
                                              expr_ids[i], excludes[i] );
      Tcl_AppendElement( tcl, str );
    }

    /* Deallocate memory */
    sig_link_delete_list( sigs, sig_size, sig_no_rm_index, FALSE );

    /* If the expr_ids array has one or more elements, deallocate the array */
    free_safe( expr_ids, (sizeof( int ) * sig_size) );
    free_safe( excludes, (sizeof( int ) * sig_size) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  PROFILE_END;

  return( retval );

}

/*! 
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.
    
 Returns the covered FSM expression line/character values for each.
*/  
int tcl_func_collect_covered_fsms(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_COVERED_FSMS);

  int        retval = TCL_OK;  /* Return value for this function */
  char       str[85];          /* Temporary string container */
  int        start_line;       /* Starting line number of this module */
  int*       expr_ids;         /* Array containing the statement IDs of all uncovered FSM signals */
  int*       excludes;         /* Array containing exclude values of all uncovered FSM signals */
  func_unit* funit;            /* Pointer to found functional unit */

  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    vsignal**    sigs            = NULL;
    unsigned int sig_size        = 0;
    unsigned int sig_no_rm_index = 1;
    unsigned int i;

    fsm_collect( funit, 1, &sigs, &sig_size, &sig_no_rm_index, &expr_ids, &excludes );

    /* Load covered FSMs into Tcl */
    for( i=0; i<sig_size; i++ ) {
      snprintf( str, 85, "%d.%d %d.%d", (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + 14),
                                        (sigs[i]->line - (start_line - 1)), (sigs[i]->suppl.part.col + ((int)strlen( sigs[i]->name ) - 1) + 15) );
      Tcl_AppendElement( tcl, str );
    }
  
    /* Deallocate memory */
    sig_link_delete_list( sigs, sig_size, sig_no_rm_index, FALSE );

    /* If the expr_ids array has one or more elements, deallocate the array */
    free_safe( expr_ids, (sizeof( int ) * sig_size) );
    free_safe( excludes, (sizeof( int ) * sig_size) );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "fsm_states", "fsm_hit_states", "fsm_arcs", "fsm_hit_arcs", "fsm_in_state" and "fsm_out_state"
 global variables with the FSM coverage information from the specified output state expression.
*/
int tcl_func_get_fsm_coverage(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_FSM_COVERAGE);

  int          retval = TCL_OK;     /* Return value for this function */
  int          expr_id;             /* Expression ID of output state expression */
  char**       total_fr_states;     /* String array containing all possible states for this FSM */
  unsigned int total_fr_state_num;  /* Number of elements in the total_states array */
  char**       total_to_states;     /* String array containing all possible states for this FSM */
  unsigned int total_to_state_num;  /* Number of elements in the total_states array */
  char**       hit_fr_states;       /* String array containing hit states for this FSM */
  unsigned int hit_fr_state_num;    /* Number of elements in the hit_states array */
  char**       hit_to_states;       /* String array containing hit states for this FSM */
  unsigned int hit_to_state_num;    /* Number of elements in the hit_states array */
  char**       total_from_arcs;     /* String array containing all possible state transition input states */
  char**       total_to_arcs;       /* String array containing all possible state transition output states */
  int*         total_ids;           /* Integer array containing exclusion IDs for each state transition */
  int*         excludes;            /* Integer array containing exclude values for each state transition */
  char**       reasons;             /* String array containing exclusion reasons */
  int          total_arc_num;       /* Number of elements in both the total_from_arcs and total_to_arcs arrays */
  char**       hit_from_arcs;       /* String array containing hit state transition input states */
  char**       hit_to_arcs;         /* String array containing hit state transition output states */
  int          hit_arc_num;         /* Number of elements in both the hit_from_arcs and hit_to_arcs arrays */
  char**       input_state;         /* String containing the input state code */
  unsigned int input_size;          /* Number of elements in the input_state array */
  char**       output_state;        /* String containing the output state code */
  unsigned int output_size;         /* Number of elements in the output_state array */
  char         str[4096];           /* Temporary string container */
  int          i;                   /* Loop iterator */
  func_unit*   funit;               /* Pointer to found functional unit */

  expr_id = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    fsm_get_coverage( funit, expr_id, &total_fr_states, &total_fr_state_num, &total_to_states, &total_to_state_num,
                      &hit_fr_states, &hit_fr_state_num, &hit_to_states, &hit_to_state_num,
                      &total_from_arcs, &total_to_arcs, &total_ids, &excludes, &reasons, &total_arc_num, &hit_from_arcs, &hit_to_arcs, &hit_arc_num,
                      &input_state, &input_size, &output_state, &output_size );

    /* Load FSM total from states into Tcl */
    for( i=0; i<total_fr_state_num; i++ ) {
      Tcl_SetVar( tcl, "fsm_in_states", total_fr_states[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( total_fr_states[i], (strlen( total_fr_states[i] ) + 1) );
    }
    free_safe( total_fr_states, (sizeof( char* ) * total_fr_state_num) );

    /* Load FSM total to states into Tcl */
    for( i=0; i<total_to_state_num; i++ ) {
      Tcl_SetVar( tcl, "fsm_out_states", total_to_states[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( total_to_states[i], (strlen( total_to_states[i] ) + 1) );
    }
    free_safe( total_to_states, (sizeof( char* ) * total_to_state_num) );

    /* Load FSM hit from states into Tcl */
    for( i=0; i<hit_fr_state_num; i++ ) {
      Tcl_SetVar( tcl, "fsm_in_hit_states", hit_fr_states[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( hit_fr_states[i], (strlen( hit_fr_states[i] ) + 1) );
    }
    free_safe( hit_fr_states, (sizeof( char* ) * hit_fr_state_num) );

    /* Load FSM hit to states into Tcl */
    for( i=0; i<hit_to_state_num; i++ ) {
      Tcl_SetVar( tcl, "fsm_out_hit_states", hit_to_states[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( hit_to_states[i], (strlen( hit_to_states[i] ) + 1) );
    }
    free_safe( hit_to_states, (sizeof( char* ) * hit_to_state_num) );

    /* Load FSM total arcs into Tcl */
    for( i=0; i<total_arc_num; i++ ) {
      snprintf( str, 4096, "%s %s %d {%s}", total_from_arcs[i], total_to_arcs[i], excludes[i], ((reasons[i] != NULL) ? reasons[i] : "") );
      Tcl_SetVar( tcl, "fsm_arcs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( total_from_arcs[i], (strlen( total_from_arcs[i] ) + 1) );
      free_safe( total_to_arcs[i], (strlen( total_to_arcs[i] ) + 1) );
      free_safe( reasons[i], (strlen( reasons[i] ) + 1) );
    }
    free_safe( total_from_arcs, (sizeof( char* ) * total_arc_num) );
    free_safe( total_to_arcs, (sizeof( char* ) * total_arc_num) );
    free_safe( total_ids, (sizeof( int ) * total_arc_num) );
    free_safe( excludes, (sizeof( int ) * total_arc_num) );
    free_safe( reasons, (sizeof( char* ) * total_arc_num) );

    /* Load FSM hit arcs into Tcl */
    for( i=0; i<hit_arc_num; i++ ) {
      snprintf( str, 4096, "%s %s", hit_from_arcs[i], hit_to_arcs[i] );
      Tcl_SetVar( tcl, "fsm_hit_arcs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( hit_from_arcs[i], (strlen( hit_from_arcs[i] ) + 1) );
      free_safe( hit_to_arcs[i], (strlen( hit_to_arcs[i] ) + 1) );
    }
    free_safe( hit_from_arcs, (sizeof( char* ) * hit_arc_num) );
    free_safe( hit_to_arcs, (sizeof( char* ) * hit_arc_num) );

    /* Load FSM input state into Tcl */
    if( input_size > 0 ) {
      Tcl_SetVar( tcl, "fsm_in_state", input_state[0], TCL_GLOBAL_ONLY );
      for( i=0; i<input_size; i++ ) {
        free_safe( input_state[i], (strlen( input_state[i] ) + 1) );
      }
      free_safe( input_state, (sizeof( char* ) * input_size) );
    }

    /* Load FSM output state into Tcl */
    if( output_size > 0 ) {
      Tcl_SetVar( tcl, "fsm_out_state", output_state[0], TCL_GLOBAL_ONLY );
      for( i=0; i<output_size; i++ ) {
        free_safe( output_state[i], (strlen( output_state[i] ) + 1) );
      }
      free_safe( output_state, (sizeof( char* ) * output_size) );
    }

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the uncovered assertion module instance names and exclude list.
*/
int tcl_func_collect_uncovered_assertions(
  ClientData  d,      /*!< Tcl structure */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_UNCOVERED_ASSERTIONS);

  int          retval = TCL_OK;  /* Return value for this function */
  char**       inst_names;       /* Array of instance names for all uncovered assertions in the specified functional unit */
  int*         excludes;         /* Array of integers specifying the exclude information for an assertion instance */
  unsigned int inst_size;        /* Number of valid elements in the uncov_inst_names/excludes arrays */
  unsigned int i;                /* Loop iterator */
  func_unit*   funit;            /* Pointer to found functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    curr_db = tcl_func_is_funit( tcl, argv[1] ) ? 1 : 0;
    assertion_collect( funit, 0, &inst_names, &excludes, &inst_size );
    curr_db = 0;

    /* Load uncovered assertions into Tcl */
    for( i=0; i<inst_size; i++ ) {
      int   str_size = strlen( inst_names[i] ) + 1 + 20 + 1;
      char* str      = (char*)malloc_safe( str_size );
      snprintf( str, str_size, "%s %d", inst_names[i], excludes[i] );
      Tcl_AppendElement( tcl, str );
      free_safe( inst_names[i], (strlen( inst_names[i] ) + 1) );
      free_safe( str, str_size );
    }

    if( inst_size > 0 ) {
      free_safe( inst_names, (sizeof( char* ) * inst_size) );
      free_safe( excludes, (sizeof( int ) * inst_size) );
    }

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  PROFILE_END;

  return( retval );
  
}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the covered assertion module instance names and exclude list.
*/
int tcl_func_collect_covered_assertions(
  ClientData  d,      /*!< Tcl structure */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_COVERED_ASSERTIONS);

  int          retval = TCL_OK;  /* Return value for this function */
  char**       inst_names;       /* Array of instance names for all uncovered assertions in the specified functional unit */
  int*         excludes;         /* Array of integers specifying the exclude information for an assertion instance */
  unsigned int inst_size;        /* Number of valid elements in the uncov_inst_names/excludes arrays */
  unsigned int i;                /* Loop iterator */
  func_unit*   funit;            /* Pointer to found functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    curr_db = tcl_func_is_funit( tcl, argv[1] ) ? 1 : 0;
    assertion_collect( funit, 1, &inst_names, &excludes, &inst_size );
    curr_db = 0;

    /* Load uncovered assertions into Tcl */
    for( i=0; i<inst_size; i++ ) {
      Tcl_AppendElement( tcl, inst_names[i] );
      free_safe( inst_names[i], (strlen( inst_names[i] ) + 1) );
    }

    if( inst_size > 0 ) {
      free_safe( inst_names, (sizeof( char* ) * inst_size) );
    }

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "assert_cov_mod" and "assert_cov_points" global variables with the coverage points from the
 given instance.
*/
int tcl_func_get_assert_coverage(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_ASSERT_COVERAGE);

  int        retval = TCL_OK;  /* Return value for this function */
  char*      inst_name;        /* Name of assertion module instance to get coverage information for */
  char       str[4096];        /* Temporary string holder */
  func_unit* funit;            /* Pointer to found functional unit */

  inst_name = strdup_safe( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    char*     assert_mod;
    str_link* cp_head;
    str_link* cp_tail;
    str_link* curr_cp;

    curr_db = tcl_func_is_funit( tcl, argv[1] ) ? 1 : 0;
    assertion_get_coverage( funit, inst_name, &assert_mod, &cp_head, &cp_tail );
    curr_db = 0;

    Tcl_SetVar( tcl, "assert_cov_mod", assert_mod, TCL_GLOBAL_ONLY );
    free_safe( assert_mod, (strlen( assert_mod ) + 1) );

    curr_cp = cp_head;
    while( curr_cp != NULL ) {
      snprintf( str, 4096, "{%s} %d %d %d {%s}", curr_cp->str, curr_cp->suppl, curr_cp->suppl2, curr_cp->suppl3, ((curr_cp->str2 != NULL) ? curr_cp->str2 : "") );
      Tcl_SetVar( tcl, "assert_cov_points", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      curr_cp = curr_cp->next;
    }

    /* Deallocate the string list */
    str_link_delete_list( cp_head );

  }

  free_safe( inst_name, (strlen( inst_name ) + 1) );

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Opens the specified CDD file, reading its contents into the database.
*/
int tcl_func_open_cdd(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_OPEN_CDD);

  int   retval = TCL_OK;  /* Return value for this function */
  char* ifile;            /* Name of CDD file to open */

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    funit_link* funitl;
    inst_link*  instl;

    ifile = strdup_safe( argv[1] );

    Try {
      report_read_cdd_and_ready( ifile );
    } Catch_anonymous {
      retval = TCL_ERROR;
    }

    free_safe( ifile, (strlen( ifile ) + 1) );

    /* Gather the functional unit instances */
    instl = db_list[0]->inst_head;
    while( instl != NULL ) {
      tcl_func_get_instances( tcl, instl->inst );
      instl = instl->next;
    }

    /* Gather the functional units */
    funitl = db_list[1]->funit_head;
    while( funitl != NULL ) {
      if( !funit_is_unnamed( funitl->funit ) &&
          ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( funit_get_curr_module_safe( funitl->funit ) )) ) {
        gui_funit_list = (func_unit**)realloc_safe( gui_funit_list, (sizeof( func_unit* ) * gui_funit_index), (sizeof( func_unit* ) * (gui_funit_index + 1)) );
        gui_funit_list[gui_funit_index] = funitl->funit;
        gui_funit_index++;
      }
      funitl = funitl->next;
    }

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Closes the current CDD file, freeing all associated memory with it.
*/
int tcl_func_close_cdd(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_CLOSE_CDD);

  int retval = TCL_OK;  /* Return value for this function */

  Try {

    report_close_cdd();

    free_safe( gui_funit_list, (sizeof( func_unit* )  * gui_funit_index) );
    gui_funit_list  = NULL;
    gui_funit_index = 0;
    
    free_safe( gui_inst_list,  (sizeof( funit_inst* ) * gui_inst_index) );
    gui_inst_list  = NULL;
    gui_inst_index = 0;

  } Catch_anonymous {
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Saves the current CDD file with the specified name.
*/
int tcl_func_save_cdd(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_SAVE_CDD);

  int   retval = TCL_OK;  /* Return value for this function */
  char* filename;         /* Name of file to save as */

  printf( "Saving CDD file %s\n", argv[1] );

  filename = strdup_safe( argv[1] );

  Try {
    report_save_cdd( filename );
  } Catch_anonymous {
    retval = TCL_ERROR;
  }

  free_safe( filename, (strlen( filename ) + 1) );

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Merges the specified CDD file with the current CDD database.
*/
int tcl_func_merge_cdd(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_MERGE_CDD);

  int   retval = TCL_OK;  /* Return value for this function */
  char* ifile;            /* Name of CDD file to merge */

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1] );
    
    /* Set the exclusion resolution value */
    if( strncmp( "first", argv[2], 5 ) == 0 ) {
      merge_er_value = MERGE_ER_FIRST;
    } else if( strncmp( "last", argv[2], 4 ) == 0 ) {
      merge_er_value = MERGE_ER_LAST;
    } else if( strncmp( "all", argv[2], 3 ) == 0 ) {
      merge_er_value = MERGE_ER_ALL;
    } else if( strncmp( "new", argv[2], 3 ) == 0 ) {
      merge_er_value = MERGE_ER_NEW;
    } else if( strncmp( "old", argv[2], 3 ) == 0 ) {
      merge_er_value = MERGE_ER_OLD;
    } else {
      merge_er_value = MERGE_ER_FIRST;
    }

    /* Add the specified merge file to the list */
    str_link_add( ifile, &merge_in_head, &merge_in_tail );
    merge_in_num++;

    Try {
      report_read_cdd_and_ready( ifile );
    } Catch_anonymous {
      retval = TCL_ERROR;
    }

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "line_summary_hit", "line_summary_excluded" and "line_summary_total" global variables with the total
 number of lines hit during simulation, the total number of excluded lines and the total number of lines
 for the specified functional unit.
*/
int tcl_func_get_line_summary(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */ 
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_LINE_SUMMARY);

  int          retval = TCL_OK;  /* Return value for this function */
  unsigned int hit;              /* Contains total number of lines hit */
  unsigned int excluded;         /* Contains total number of lines excluded */
  unsigned int total;            /* Contains total number of lines evaluated */
  char         value[20];        /* String version of a value */
  func_unit*   funit  = NULL;    /* Pointer to found functional unit */
  funit_inst*  inst   = NULL;    /* Pointer to found functional unit instance */

  if( tcl_func_is_funit( tcl, argv[1] ) ) {
    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
      line_get_funit_summary( funit, &hit, &excluded, &total );
    } else {
      snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find specified functional unit" );
    }
  } else {
    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {
      line_get_inst_summary( inst, &hit, &excluded, &total );
    } else {
      snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find specified functional unit instance" );
    }
  }

  if( (funit != NULL) || (inst != NULL) ) {
    snprintf( value, 20, "%u", hit );
    Tcl_SetVar( tcl, "line_summary_hit", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", excluded );
    Tcl_SetVar( tcl, "line_summary_excluded", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", total );
    Tcl_SetVar( tcl, "line_summary_total", value, TCL_GLOBAL_ONLY );
  } else {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "toggle_summary_total" and "toggle_summary_hit" to the total number
 of signals evaluated for toggle coverage and the total number of signals with complete toggle coverage
 for the specified functional unit.
*/
int tcl_func_get_toggle_summary(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_TOGGLE_SUMMARY);

  int          retval = TCL_OK;  /* Return value for this function */
  unsigned int hit;              /* Contains total number of signals hit */
  unsigned int excluded;         /* Contains number of excluded toggles */
  unsigned int total;            /* Contains total number of signals evaluated */
  char         value[20];        /* String version of a value */
  func_unit*   funit  = NULL;    /* Pointer to found functional unit */
  funit_inst*  inst   = NULL;    /* Pointer to found functional unit instance */

  if( tcl_func_is_funit( tcl, argv[1] ) ) {
    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
      toggle_get_funit_summary( funit, &hit, &excluded, &total );
    } else {
      snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find specified functional unit" );
    }
  } else {
    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {
      toggle_get_inst_summary( inst, &hit, &excluded, &total );
    } else {
      snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find specified functional unit instance" );
    }
  }

  if( (funit != NULL) || (inst != NULL) ) {
    snprintf( value, 20, "%u", hit );
    Tcl_SetVar( tcl, "toggle_summary_hit", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", excluded );
    Tcl_SetVar( tcl, "toggle_summary_excluded", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", total );
    Tcl_SetVar( tcl, "toggle_summary_total", value, TCL_GLOBAL_ONLY );
  } else {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "memory_summary_total" and "memory_summary_hit" to the total number
 of signals evaluated for memory coverage and the total number of signals with complete memory coverage
 for the specified functional unit.
*/
int tcl_func_get_memory_summary(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_MEMORY_SUMMARY);

  int          retval = TCL_OK;  /* Return value for this function */
  unsigned int hit;              /* Contains total number of signals hit */
  unsigned int excluded;         /* Number of excluded memory coverage points */
  unsigned int total;            /* Contains total number of signals evaluated */
  char         value[20];        /* String version of a value */
  func_unit*   funit  = NULL;    /* Pointer to found functional unit */
  funit_inst*  inst   = NULL;    /* Pointer to found functional unit instance */

  if( tcl_func_is_funit( tcl, argv[1] ) ) {
    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
      memory_get_funit_summary( funit, &hit, &excluded, &total );
    } else {
      strcpy( user_msg, "Internal Error:  Unable to find specified functional unit" );
    }
  } else {
    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {
      memory_get_inst_summary( inst, &hit, &excluded, &total );
    } else {
      strcpy( user_msg, "Internal Error:  Unable to find specified functional unit instance" );
    }
  }

  if( (funit != NULL) || (inst != NULL) ) {
    snprintf( value, 20, "%u", hit );
    Tcl_SetVar( tcl, "memory_summary_hit", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", excluded );
    Tcl_SetVar( tcl, "memory_summary_excluded", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", total );
    Tcl_SetVar( tcl, "memory_summary_total", value, TCL_GLOBAL_ONLY );
  } else {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "comb_summary_total" and "comb_summary_hit" to the total number
 of expression values evaluated for combinational logic coverage and the total number of expression values
 with complete combinational logic coverage for the specified functional unit.
*/
int tcl_func_get_comb_summary(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_COMB_SUMMARY);

  int          retval = TCL_OK;  /* Return value for this function */
  unsigned int hit;              /* Contains total number of expressions hit */
  unsigned int excluded;         /* Number of excluded logical combinations */
  unsigned int total;            /* Contains total number of expressions evaluated */
  char         value[20];        /* String version of a value */
  func_unit*   funit  = NULL;    /* Pointer to found functional unit */
  funit_inst*  inst   = NULL;    /* Pointer to found functional unit instance */

  if( tcl_func_is_funit( tcl, argv[1] ) ) {
    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
      combination_get_funit_summary( funit, &hit, &excluded, &total );
    } else {
      strcpy( user_msg, "Internal Error:  Unable to find specified functional unit" );
    }
  } else {
    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {
      combination_get_inst_summary( inst, &hit, &excluded, &total );
    } else {
      strcpy( user_msg, "Internal Error:  Unable to find specified functional unit instance" );
    }
  }

  if( (funit != NULL) || (inst != NULL) ) {
    snprintf( value, 20, "%u", hit );
    Tcl_SetVar( tcl, "comb_summary_hit", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", excluded );
    Tcl_SetVar( tcl, "comb_summary_excluded", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", total );
    Tcl_SetVar( tcl, "comb_summary_total", value, TCL_GLOBAL_ONLY );
  } else {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "fsm_summary_total" and "fsm_summary_hit" to the total number
 of state transitions evaluated for FSM coverage and the total number of state transitions
 with complete FSM state transition coverage for the specified functional unit.
*/
int tcl_func_get_fsm_summary(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_FSM_SUMMARY);

  int          retval = TCL_OK;  /* Return value for this function */
  int          hit;              /* Contains total number of expressions hit */
  int          excluded;         /* Number of excluded arcs */
  int          total;            /* Contains total number of expressions evaluated */
  char         value[20];        /* String version of a value */
  func_unit*   funit  = NULL;    /* Pointer to found functional unit */
  funit_inst*  inst   = NULL;    /* Pointer to found functional unit instance */

  if( tcl_func_is_funit( tcl, argv[1] ) ) {
    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
      fsm_get_funit_summary( funit, &hit, &excluded, &total );
    } else {
      strcpy( user_msg, "Internal Error:  Unable to find specified functional unit" );
    }
  } else {
    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {
      fsm_get_inst_summary( inst, &hit, &excluded, &total );
    } else {
      strcpy( user_msg, "Internal Error:  Unable to find specified functional unit instance" );
    }
  }

  if( (funit != NULL) || (inst != NULL) ) {
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "fsm_summary_hit", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", excluded );
    Tcl_SetVar( tcl, "fsm_summary_excluded", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "fsm_summary_total", value, TCL_GLOBAL_ONLY );
  } else {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "assert_summary_total" and "assert_summary_hit" to the total number
 of assertions evaluated for coverage and the total number of hit assertions for the specified functional unit.
*/
int tcl_func_get_assert_summary(
  ClientData  d,      /*!< Tcl data container */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_ASSERT_SUMMARY);

  int          retval = TCL_OK;  /* Return value for this function */
  unsigned int hit;              /* Contains total number of expressions hit */
  unsigned int excluded;         /* Number of excluded assertions */
  unsigned int total;            /* Contains total number of expressions evaluated */
  char         value[20];        /* String version of a value */
  func_unit*   funit;            /* Pointer to found functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    curr_db = tcl_func_is_funit( tcl, argv[1] ) ? 1 : 0;
    assertion_get_funit_summary( funit, &hit, &excluded, &total );
    curr_db = 0;

    snprintf( value, 20, "%u", hit );
    Tcl_SetVar( tcl, "assert_summary_hit", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", excluded );
    Tcl_SetVar( tcl, "assert_summary_excluded", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%u", total );
    Tcl_SetVar( tcl, "assert_summary_total", value, TCL_GLOBAL_ONLY );

  } else {

    strcpy( user_msg, "Internal Error:  Unable to find functional unit in design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Preprocesses the specified filename, outputting the contents into a temporary file whose name is passed back
 to the calling function.
*/
int tcl_func_preprocess_verilog(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_PREPROCESS_VERILOG);

  int       retval = TCL_OK;  /* Return value for this function */
  char*     ppfilename;       /* Preprocessed filename to return to calling function */
  FILE*     out;              /* File handle to preprocessed file */
  str_link* arg;
  
  /* Add all of the include and define score arguments before calling the preprocessor */
  arg = score_args_head;
  while( arg != NULL) {
    if( strcmp( "-D", arg->str ) == 0 ) {
      score_parse_define( arg->str2 );
    } else if( strcmp( "-I", arg->str ) == 0 ) {
      search_add_include_path( arg->str2 );
    }
    arg = arg->next;
  }

  /* Create temporary output filename */
  ppfilename = Tcl_Alloc( 10 );
  snprintf( ppfilename, 10, "tmpXXXXXX" );
  assert( mkstemp( ppfilename ) != 0 );

  out = fopen( ppfilename, "w" );

  Try {

    if( out == NULL ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open temporary file %s for writing", ppfilename );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;
    }

    /* Now the preprocessor on this file first */
    if( strcmp( argv[1], "NA" ) == 0 ) {
      fprintf( out, "No information available\n" );
    } else {
      reset_pplexer( argv[1], out );
      PPVLlex();
    }

  } Catch_anonymous {
    retval = TCL_ERROR;
  }

  fclose( out );
  
  /* Set the return value to the name of the temporary file */
  Tcl_SetResult( tcl, ppfilename, TCL_DYNAMIC );

  return( retval );
  
}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the score directory pathname to the calling Tcl process.
*/
int tcl_func_get_score_path(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_SCORE_PATH);

  int retval = TCL_OK;  /* Return value for this function */

  Tcl_SetResult( tcl, score_run_path, TCL_STATIC );

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the full pathname of the specified included file.  Uses the -I options specified in the CDD file
 for reference.
*/
int tcl_func_get_include_pathname(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_INCLUDE_PATHNAME);

  int  retval = TCL_OK;  /* Return value for this function */
  char incpath[4096];    /* Contains full included pathname */
  
  strcpy( incpath, argv[1] );

  while( !file_exists( incpath ) && (retval == TCL_OK) ) {
    
    /* Find an include path from the score args */
    str_link* arg = score_args_head;
    while( (arg != NULL) && (strcmp( "-I", arg->str ) != 0) ) {
      arg = arg->next;
    }
    if( arg == NULL ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to find included file \"%s\"", argv[1] );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    } else {

      /* Test include score arg for absolute pathname */
      snprintf( incpath, 4096, "%s/%s", arg->str2, argv[1] );

      /* If the absolute pathname is not valid, try a relative pathname */
      if( !file_exists( incpath ) ) {
        snprintf( incpath, 4096, "%s/%s/%s", score_run_path, arg->str2, argv[1] );
      }
    }

  }

  if( retval == TCL_OK ) {
    Tcl_SetResult( tcl, incpath, TCL_STATIC );
  }

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the generation specified for the given functional unit.
*/
int tcl_func_get_generation(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_GENERATION);

  int       retval = TCL_OK;    /* Return value for this function */
  char      generation[2];      /* Generation to use for the specified module */
  char*     funit_name;         /* Name of functional unit to find generation for */
  bool      mod_found = FALSE;  /* Set to TRUE if we found a generation for this exact module */
  str_link* arg;

  funit_name = strdup_safe( argv[1] );
  strcpy( generation, "3" );

  /* Search the entire command-line */
  arg = score_args_head;
  while( arg != NULL ) {

    /* Find a generation argument in the score args */
    while( (arg != NULL) && (strcmp( "-g", arg->str ) != 0) ) {
      arg = arg->next;
    }

    if( arg != NULL ) {
      if( (strlen( arg->str2 ) == 1) && !mod_found ) {
        generation[0] = arg->str2[(strlen( arg->str2 ) - 1)];
      } else if( ((strlen( arg->str2 ) - 2) == strlen( funit_name )) &&
                 (strncmp( funit_name, arg->str2, strlen( funit_name ) ) == 0) ) {
        generation[0] = arg->str2[(strlen( arg->str2 ) - 1)];
        mod_found     = TRUE;
      }
      arg = arg->next; 
    }

  }

  if( retval == TCL_OK ) {
    Tcl_SetResult( tcl, generation, TCL_STATIC );
  }

  free_safe( funit_name, (strlen( funit_name ) + 1) );

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_line_exclude(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_SET_LINE_EXCLUDE);

  int         retval = TCL_OK;  /* Return value for this function */
  func_unit*  funit  = NULL;    /* Pointer to found functional unit */
  funit_inst* inst   = NULL;    /* Pointer to found functional unit instance */
  int         line;
  int         value;
  char*       reason;           /* Exclusion reason from command-line */

  line   = atoi( argv[2] );
  value  = atoi( argv[3] );
  reason = exclude_format_reason( argv[4] );

  /* If the current block is a functional unit, deal with the functional unit database */
  if( tcl_func_is_funit( tcl, argv[1] ) ) {

    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

      unsigned int i;

      /* Set the line exclusion value for the functional unit database */
      exclude_set_line_exclude( funit, line, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, funit->stat );

      /* Now set the line exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->suppl.part.type == funit->suppl.part.type) ) {
          exclude_set_line_exclude( gui_inst_list[i]->funit, line, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, gui_inst_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit" );
    }

  /* Otherwise, deal with the functional unit instance database */
  } else {

    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {

      unsigned int i = gui_inst_index;

      /* Set the line exclusion value for the instance database */
      exclude_set_line_exclude( inst->funit, line, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, inst->stat );

      /* If we are attempting to exclude the line, check all other instances -- if they all exclude this line, exclude the line from the functional unit */
      if( value == 1 ) {

        /* Check the line exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->suppl.part.type != inst->funit->suppl.part.type) ||
                exclude_is_line_excluded( gui_inst_list[i]->funit, line )) ) i++;

      }

      /* If the line has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->suppl.part.type != inst->funit->suppl.part.type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_line_exclude( gui_funit_list[i], line, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, gui_funit_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit instance" );
    }

  }

  if( (funit == NULL) && (inst == NULL) ) {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( reason, (strlen( reason ) + 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_toggle_exclude(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_SET_TOGGLE_EXCLUDE);

  int         retval = TCL_OK;  /* Return value for this function */
  func_unit*  funit  = NULL;    /* Pointer to found functional unit */
  funit_inst* inst   = NULL;    /* Pointer to found functional unit instance */
  char*       sig_name;         /* Name of signal being excluded/included */
  int         value;            /* Exclude value (0 or 1) */
  char*       reason;           /* Reason for the exclusion */

  sig_name = strdup_safe( argv[2] );
  value    = atoi( argv[3] );
  reason   = exclude_format_reason( argv[4] );

  /* If the current block is a functional unit, deal with the functional unit database */
  if( tcl_func_is_funit( tcl, argv[1] ) ) {

    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

      unsigned int i;

      /* Set the toggle exclusion value for the functional unit database */
      exclude_set_toggle_exclude( funit, sig_name, value, 'T', ((reason != NULL) ? strdup_safe( reason ) : NULL), funit->stat );

      /* Now set the toggle exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->suppl.part.type == funit->suppl.part.type) ) {
          exclude_set_toggle_exclude( gui_inst_list[i]->funit, sig_name, value, 'T', ((reason != NULL) ? strdup_safe( reason ) : NULL), gui_inst_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit" );
    }

  /* Otherwise, deal with the functional unit instance database */
  } else {

    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {

      unsigned int i = gui_inst_index;

      /* Set the toggle exclusion value for the instance database */
      exclude_set_toggle_exclude( inst->funit, sig_name, value, 'T', ((reason != NULL) ? strdup_safe( reason ) : NULL), inst->stat );

      /* If we are attempting to exclude the signal, check all other instances -- if they all exclude this signal, exclude the signal from the functional unit */
      if( value == 1 ) {

        /* Check the toggle exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->suppl.part.type != inst->funit->suppl.part.type) ||
                exclude_is_toggle_excluded( gui_inst_list[i]->funit, sig_name )) ) i++;

      }

      /* If the signal has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->suppl.part.type != inst->funit->suppl.part.type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_toggle_exclude( gui_funit_list[i], sig_name, value, 'T', ((reason != NULL) ? strdup_safe( reason ) : NULL), gui_funit_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit instance" );
    }

  }

  if( (funit == NULL) && (inst == NULL) ) {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( sig_name, (strlen( sig_name ) + 1) );
  free_safe( reason, (strlen( reason ) + 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified memory.  This function is called whenever the user changes
 the exclusion for a specified memory.  The tcl_func_get_memory_summary function should be called
 immediately after to get the new memory summary information.
*/
int tcl_func_set_memory_exclude(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_SET_MEMORY_EXCLUDE);

  int         retval = TCL_OK;  /* Return value for this function */
  func_unit*  funit  = NULL;    /* Pointer to found functional unit */
  funit_inst* inst   = NULL;    /* Pointer to found functional unit instance */
  char*       sig_name;         /* Name of signal to exclude */
  int         value;            /* Exclusion value from command-line */
  char*       reason;           /* Reason for exclusion */

  sig_name = strdup_safe( argv[2] );
  value    = atoi( argv[3] );
  reason   = exclude_format_reason( argv[4] );

  /* If the current block is a functional unit, deal with the functional unit database */
  if( tcl_func_is_funit( tcl, argv[1] ) ) {

    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

      unsigned int i;

      /* Set the memory exclusion value for the functional unit database */
      exclude_set_toggle_exclude( funit, sig_name, value, 'M', ((reason != NULL) ? strdup_safe( reason ) : NULL), funit->stat );

      /* Now set the memory exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->suppl.part.type == funit->suppl.part.type) ) {
          exclude_set_toggle_exclude( gui_inst_list[i]->funit, sig_name, value, 'M', ((reason != NULL) ? strdup_safe( reason ) : NULL), gui_inst_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit" );
    }

  /* Otherwise, deal with the functional unit instance database */
  } else {

    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {

      unsigned int i = gui_inst_index;

      /* Set the memory exclusion value for the instance database */
      exclude_set_toggle_exclude( inst->funit, sig_name, value, 'M', ((reason != NULL) ? strdup_safe( reason ) : NULL), inst->stat );

      /* If we are attempting to exclude the signal, check all other instances -- if they all exclude this signal, exclude the signal from the functional unit */
      if( value == 1 ) {

        /* Check the toggle exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->suppl.part.type != inst->funit->suppl.part.type) ||
                exclude_is_toggle_excluded( gui_inst_list[i]->funit, sig_name )) ) i++;

      }

      /* If the signal has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->suppl.part.type != inst->funit->suppl.part.type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_toggle_exclude( gui_funit_list[i], sig_name, value, 'M', ((reason != NULL) ? strdup_safe( reason ) : NULL), gui_funit_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit instance" );
    }

  }

  if( (funit == NULL) && (inst == NULL) ) {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( sig_name, (strlen( sig_name ) + 1) );
  free_safe( reason, (strlen( reason ) + 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_comb_exclude(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_SET_COMB_EXCLUDE);

  int         retval = TCL_OK;  /* Return value for this function */
  funit_inst* inst   = NULL;    /* Pointer to found functional unit instance */
  func_unit*  funit  = NULL;    /* Pointer to found functional unit */
  int         expr_id;
  int         uline_id;
  int         value;
  char*       reason;           /* Exclusion reason */

  expr_id  = atoi( argv[2] );
  uline_id = atoi( argv[3] );
  value    = atoi( argv[4] );
  reason   = exclude_format_reason( argv[5] );

  /* If the current block is a functional unit, deal with the functional unit database */
  if( tcl_func_is_funit( tcl, argv[1] ) ) {

    /* Search for functional unit */
    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {
    
      unsigned int i;

      /* Set the combinational logic exclusion value for the functional unit database */
      exclude_set_comb_exclude( funit, expr_id, uline_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, funit->stat );

      /* Now set the combinational logic exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->suppl.part.type == funit->suppl.part.type) ) {
          exclude_set_comb_exclude( gui_inst_list[i]->funit, expr_id, uline_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, gui_inst_list[i]->stat ); 
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit" );
    }

  /* Otherwise, deal with the functional unit instance database */
  } else {

    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {

      unsigned int i = gui_inst_index;

      /* Set the combinational logic exclusion value for the instance database */
      exclude_set_comb_exclude( inst->funit, expr_id, uline_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, inst->stat );

      /* If we are attempting to exclude the expression, check all other instances -- if they all exclude this expression, exclude the expression from the functional unit */
      if( value == 1 ) {

        /* Check the combinational logic exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->suppl.part.type != inst->funit->suppl.part.type) ||
                exclude_is_comb_excluded( gui_inst_list[i]->funit, expr_id, uline_id )) ) i++;

      }

      /* If the expression has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->suppl.part.type != inst->funit->suppl.part.type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_comb_exclude( gui_funit_list[i], expr_id, uline_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, gui_funit_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit instance" );
    }

  }

  if( (funit == NULL) && (inst == NULL) ) {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( reason, (strlen( reason ) + 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_fsm_exclude(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_FSM_EXCLUDE);

  int         retval = TCL_OK;  /* Return value for this function */
  func_unit*  funit  = NULL;    /* Pointer to found functional unit */
  funit_inst* inst   = NULL;    /* Pointer to found functional unit instance */
  int         expr_id;
  char*       from_state;
  char*       to_state;
  int         value;
  char*       reason;           /* Exclusion reason */

  expr_id    = atoi( argv[2] );
  from_state = strdup_safe( argv[3] );
  to_state   = strdup_safe( argv[4] );
  value      = atoi( argv[5] );
  reason     = exclude_format_reason( argv[6] );

  /* If the current block is a functional unit, deal with the functional unit database */
  if( tcl_func_is_funit( tcl, argv[1] ) ) {

    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

      unsigned int i;

      /* Set the combinational logic exclusion value for the functional unit database */
      exclude_set_fsm_exclude( funit, expr_id, from_state, to_state, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), funit->stat );

      /* Now set the combinational logic exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->suppl.part.type == funit->suppl.part.type) ) {
          exclude_set_fsm_exclude( gui_inst_list[i]->funit, expr_id, from_state, to_state, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), gui_inst_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit" );
    }

  /* Otherwise, deal with the functional unit instance database */
  } else {

    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {

      unsigned int i = gui_inst_index;
  
      /* Set the combinational logic exclusion value for the instance database */
      exclude_set_fsm_exclude( inst->funit, expr_id, from_state, to_state, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), inst->stat );
  
      /* If we are attempting to exclude the expression, check all other instances -- if they all exclude this expression, exclude the expression from the functional unit */
      if( value == 1 ) {
  
        /* Check the combinational logic exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->suppl.part.type != inst->funit->suppl.part.type) ||
                exclude_is_fsm_excluded( gui_inst_list[i]->funit, expr_id, from_state, to_state )) ) i++;

      }

      /* If the expression has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->suppl.part.type != inst->funit->suppl.part.type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_fsm_exclude( gui_funit_list[i], expr_id, from_state, to_state, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), gui_funit_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit instance" );
    }

  }

  if( (funit == NULL) && (inst == NULL) ) {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( from_state, (strlen( from_state ) + 1) );
  free_safe( to_state,   (strlen( to_state )   + 1) );
  free_safe( reason,     (strlen( reason ) + 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_assert_exclude(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_SET_ASSERT_EXCLUDE);

  int         retval = TCL_OK;  /* Return value for this function */
  func_unit*  funit  = NULL;    /* Pointer to found functional unit */
  funit_inst* inst   = NULL;    /* Pointer to found functional unit instance */
  char*       inst_name;
  int         expr_id;
  int         value;
  char*       reason;           /* Exclusion reason from command-line */

  inst_name = strdup_safe( argv[2] );
  expr_id   = atoi( argv[3] );
  value     = atoi( argv[4] );
  reason    = exclude_format_reason( argv[5] );

  /* If the current block is a functional unit, deal with the functional unit database */
  if( tcl_func_is_funit( tcl, argv[1] ) ) {

    if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

      unsigned int i;

      /* Set the combinational logic exclusion value for the functional unit database */
      curr_db = 1;
      exclude_set_assert_exclude( funit, inst_name, expr_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, funit->stat );

      /* Now set the combinational logic exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->suppl.part.type == funit->suppl.part.type) ) {
          curr_db = 0;
          exclude_set_assert_exclude( gui_inst_list[i]->funit, inst_name, expr_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, gui_inst_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit" );
    }

  /* Otherwise, deal with the functional unit instance database */
  } else {

    if( (inst = tcl_func_get_inst( tcl, argv[1] )) != NULL ) {

      unsigned int i = gui_inst_index;
  
      /* Set the combinational logic exclusion value for the instance database */
      curr_db = 0;
      exclude_set_assert_exclude( inst->funit, inst_name, expr_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, inst->stat );
  
      /* If we are attempting to exclude the expression, check all other instances -- if they all exclude this expression, exclude the expression from the functional unit */
      if( value == 1 ) {
  
        /* Check the combinational logic exclusion for all instances that match our functional unit */
        i       = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->suppl.part.type != inst->funit->suppl.part.type) ||
                exclude_is_assert_excluded( gui_inst_list[i]->funit, inst_name, expr_id )) ) i++;

      }

      /* If the expression has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->suppl.part.type != inst->funit->suppl.part.type)) ) i++;
        if( i < gui_funit_index ) {
          curr_db = 1;
          exclude_set_assert_exclude( gui_funit_list[i], inst_name, expr_id, value, ((reason != NULL) ? strdup_safe( reason ) : NULL), TRUE, TRUE, gui_funit_list[i]->stat );
        }
      }

    } else {
      strcpy( user_msg, "Internal Error:  Unable to find functional unit instance" );
    }

  }
  curr_db = 0;

  if( (funit == NULL) && (inst == NULL) ) {
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( inst_name, (strlen( inst_name ) + 1) );
  free_safe( reason, (strlen( reason ) + 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Generates an ASCII report based on the provided parameters.
*/
int tcl_func_generate_report(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GENERATE_REPORT);

  int   retval = TCL_OK;  /* Return value for this function */
  FILE* ofile;            /* Pointer to opened report file */

  /* Get arguments */
  Try {

    (void)report_parse_args( argc, 0, argv );

    assert( output_file != NULL );

    /* Open output stream */
    if( (ofile = fopen( output_file, "w" )) == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open report output file %s for writing", output_file );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      report_print_header( ofile );

      /* Set the current database correctly */
      curr_db = report_instance ? 0 : 1;

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

      fclose( ofile );

      snprintf( user_msg, USER_MSG_LENGTH, "Successfully generated report file %s", output_file );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );

      free_safe( output_file, (strlen( output_file ) + 1) );

    }

  } Catch_anonymous {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Incorrect parameters to report command" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  return( retval );

}

/*!
 Initializes the Tcl interpreter with all procs that are created in this file.  Also sets some global
 variables that come from the environment, the configuration execution or the Covered define file.
*/
void tcl_func_initialize(
  Tcl_Interp* tcl,        /*!< Pointer to Tcl interpreter structure */
  const char* program,    /*!< Name of Covered program provided by command-line */
  const char* user_home,  /*!< Name of user's home directory (used to store configuration file information to) */
  const char* home,       /*!< Name of Tcl script home directory (from running the configure script) */
  const char* version,    /*!< Current version of Covered being run */
  const char* browser,    /*!< Name of browser executable to use for displaying help information */
  const char* input_cdd   /*!< Name of input CDD file (if it exists) */
) { PROFILE(TCL_FUNC_INITIALIZE);
 
  Tcl_CreateCommand( tcl, "tcl_func_get_race_reason_msgs",         (Tcl_CmdProc*)(tcl_func_get_race_reason_msgs),         0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_funit_list",               (Tcl_CmdProc*)(tcl_func_get_funit_list),               0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_instance_list",            (Tcl_CmdProc*)(tcl_func_get_instance_list),            0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_funit_name",               (Tcl_CmdProc*)(tcl_func_get_funit_name),               0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_filename",                 (Tcl_CmdProc*)(tcl_func_get_filename),                 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_inst_scope",               (Tcl_CmdProc*)(tcl_func_get_inst_scope),               0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_lines",      (Tcl_CmdProc*)(tcl_func_collect_uncovered_lines),      0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_lines",        (Tcl_CmdProc*)(tcl_func_collect_covered_lines),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_race_lines",           (Tcl_CmdProc*)(tcl_func_collect_race_lines),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_toggles",    (Tcl_CmdProc*)(tcl_func_collect_uncovered_toggles),    0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_toggles",      (Tcl_CmdProc*)(tcl_func_collect_covered_toggles),      0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_memories",   (Tcl_CmdProc*)(tcl_func_collect_uncovered_memories),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_memories",     (Tcl_CmdProc*)(tcl_func_collect_covered_memories),     0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_combs",      (Tcl_CmdProc*)(tcl_func_collect_uncovered_combs),      0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_combs",        (Tcl_CmdProc*)(tcl_func_collect_covered_combs),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_fsms",       (Tcl_CmdProc*)(tcl_func_collect_uncovered_fsms),       0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_fsms",         (Tcl_CmdProc*)(tcl_func_collect_covered_fsms),         0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_assertions", (Tcl_CmdProc*)(tcl_func_collect_uncovered_assertions), 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_assertions",   (Tcl_CmdProc*)(tcl_func_collect_covered_assertions),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_funit_start_and_end",      (Tcl_CmdProc*)(tcl_func_get_funit_start_and_end),      0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_toggle_coverage",          (Tcl_CmdProc*)(tcl_func_get_toggle_coverage),          0, 0 ); 
  Tcl_CreateCommand( tcl, "tcl_func_get_memory_coverage",          (Tcl_CmdProc*)(tcl_func_get_memory_coverage),          0, 0 ); 
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_expression",          (Tcl_CmdProc*)(tcl_func_get_comb_expression),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_coverage",            (Tcl_CmdProc*)(tcl_func_get_comb_coverage),            0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_fsm_coverage",             (Tcl_CmdProc*)(tcl_func_get_fsm_coverage),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_assert_coverage",          (Tcl_CmdProc*)(tcl_func_get_assert_coverage),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_open_cdd",                     (Tcl_CmdProc*)(tcl_func_open_cdd),                     0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_close_cdd",                    (Tcl_CmdProc*)(tcl_func_close_cdd),                    0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_save_cdd",                     (Tcl_CmdProc*)(tcl_func_save_cdd),                     0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_merge_cdd",                    (Tcl_CmdProc*)(tcl_func_merge_cdd),                    0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_line_summary",             (Tcl_CmdProc*)(tcl_func_get_line_summary),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_toggle_summary",           (Tcl_CmdProc*)(tcl_func_get_toggle_summary),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_memory_summary",           (Tcl_CmdProc*)(tcl_func_get_memory_summary),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_summary",             (Tcl_CmdProc*)(tcl_func_get_comb_summary),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_fsm_summary",              (Tcl_CmdProc*)(tcl_func_get_fsm_summary),              0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_assert_summary",           (Tcl_CmdProc*)(tcl_func_get_assert_summary),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_preprocess_verilog",           (Tcl_CmdProc*)(tcl_func_preprocess_verilog),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_score_path",               (Tcl_CmdProc*)(tcl_func_get_score_path),               0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_include_pathname",         (Tcl_CmdProc*)(tcl_func_get_include_pathname),         0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_generation",               (Tcl_CmdProc*)(tcl_func_get_generation),               0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_line_exclude",             (Tcl_CmdProc*)(tcl_func_set_line_exclude),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_toggle_exclude",           (Tcl_CmdProc*)(tcl_func_set_toggle_exclude),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_memory_exclude",           (Tcl_CmdProc*)(tcl_func_set_memory_exclude),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_comb_exclude",             (Tcl_CmdProc*)(tcl_func_set_comb_exclude),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_fsm_exclude",              (Tcl_CmdProc*)(tcl_func_set_fsm_exclude),              0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_assert_exclude",           (Tcl_CmdProc*)(tcl_func_set_assert_exclude),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_generate_report",              (Tcl_CmdProc*)(tcl_func_generate_report),              0, 0 );

  /* Provide the pathname to this covered executable */
  Tcl_SetVar( tcl, "COVERED", program, TCL_GLOBAL_ONLY );
  
  /* Set the USER_HOME variable to location of user's home directory */
  Tcl_SetVar( tcl, "USER_HOME", user_home, TCL_GLOBAL_ONLY );

  /* Set HOME variable to location of scripts */
  Tcl_SetVar( tcl, "HOME", home, TCL_GLOBAL_ONLY );

  /* Set VERSION variable */
  Tcl_SetVar( tcl, "VERSION", version, TCL_GLOBAL_ONLY );

  /* Set BROWSER variable to locate browser to use for help pages if one has been specified */
  if( browser != NULL ) {
    Tcl_SetVar( tcl, "BROWSER", browser, TCL_GLOBAL_ONLY );
  }

  /* Set the input CDD file if it exists */
  if( input_cdd != NULL ) {
    Tcl_SetVar( tcl, "input_cdd", input_cdd, TCL_GLOBAL_ONLY );
  }

}
#endif

