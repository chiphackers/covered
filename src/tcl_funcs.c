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
extern char**       score_args;
extern int          score_arg_num;
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

  if( !funit_is_unnamed( root->funit ) &&
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
    Tcl_SetResult( tcl, funit->filename, TCL_STATIC );
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
  int        line_cnt;         /* Number of elements in the lines and excludes arrays */
  int        line_size;        /* Number of elements allocated in lines and excludes arrays */
  int        i;                /* Loop iterator */
  char       str[50];          /* Temporary string container */
  func_unit* funit;            /* Pointer to found functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    line_collect( funit, 0, &lines, &excludes, &line_cnt, &line_size );

    for( i=0; i<line_cnt; i++ ) {
      snprintf( str, 50, "%d %d", lines[i], excludes[i] );
      Tcl_AppendElement( tcl, str );
    }

    free_safe( lines, (sizeof( int ) * line_size) );
    free_safe( excludes, (sizeof( int ) * line_size) );

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
  int        line_cnt;          /* Number of elements in the lines and excludes arrays */
  int        line_size;         /* Number of elements allocated in the lines and excludes arrays */
  int        i;                 /* Loop iterator */
  char       str[50];           /* Temporary string container */
  func_unit* funit;             /* Pointer to found functional unit */

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    line_collect( funit, 1, &lines, &excludes, &line_cnt, &line_size );

    for( i=0; i<line_cnt; i++ ) {
      snprintf( str, 50, "%d %d", lines[i], excludes[i] );
      Tcl_AppendElement( tcl, str );
    }

    free_safe( lines, (sizeof( int ) * line_size) );
    free_safe( excludes, (sizeof( int ) * line_size) );

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
  sig_link*  sig_head = NULL;    /* Pointer to head of signal list */
  sig_link*  sig_tail = NULL;    /* Pointer to tail of signal list */
  sig_link*  sigl;               /* Pointer to current signal link being evaluated */
  char       tmp[120];           /* Temporary string */
  int        start_line;         /* Starting line number */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this command */
  start_line = atoi( argv[2] );

  /* Find all signals that did not achieve 100% coverage */
  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    toggle_collect( funit, 0, &sig_head, &sig_tail );

    sigl = sig_head;
    while( sigl != NULL ) {
      snprintf( tmp, 120, "{%d.%d %d.%d} %d",
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15),
                sigl->sig->suppl.part.excluded );
      Tcl_AppendElement( tcl, tmp );
      sigl = sigl->next;
    }

    /* Deallocate signal list (without destroying signals) */
    sig_link_delete_list( sig_head, FALSE );

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
  sig_link*  sig_head = NULL;    /* Pointer to head of signal list */
  sig_link*  sig_tail = NULL;    /* Pointer to tail of signal list */
  sig_link*  sigl;               /* Pointer to current signal being evaluated */
  char       tmp[85];            /* Temporary string */
  int        start_line;         /* Starting line number of this functional unit */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this function call */
  start_line = atoi( argv[2] );

  /* Get the toggle information for all covered signals */
  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    toggle_collect( funit, 1, &sig_head, &sig_tail );

    sigl = sig_head;
    while( sigl != NULL ) {
      snprintf( tmp, 85, "%d.%d %d.%d",
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15) );
      Tcl_AppendElement( tcl, tmp );
      sigl = sigl->next;
    }

    /* Deallocate list of signals (without deallocating the signals themselves) */
    sig_link_delete_list( sig_head, FALSE );

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
  sig_link*  sig_head = NULL;    /* Pointer to head of uncovered signal list */
  sig_link*  sig_tail = NULL;    /* Pointer to tail of uncovered signal list */
  sig_link*  sigl;               /* Pointer to current signal being evaluated */
  char       tmp[120];           /* Temporary string */
  int        start_line;         /* Starting line number of this functional unit */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this function call */
  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    /* Get the memory information for all uncovered signals */
    memory_collect( funit, 0, &sig_head, &sig_tail );

    /* Populate uncovered_memories array */
    sigl = sig_head;
    while( sigl != NULL ) {
      snprintf( tmp, 120, "%d.%d %d.%d %d",
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15),
                sigl->sig->suppl.part.excluded );
      Tcl_AppendElement( tcl, tmp );
      sigl = sigl->next;
    }

    /* Deallocate list of signals (without deallocating the signals themselves) */
    sig_link_delete_list( sig_head, FALSE );

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
  sig_link*  sig_head = NULL;    /* Pointer to head of covered signal list */
  sig_link*  sig_tail = NULL;    /* Pointer to tail of covered signal list */
  sig_link*  sigl;               /* Pointer to current signal being evaluated */
  char       tmp[120];           /* Temporary string */
  int        start_line;         /* Starting line number of this functional unit */
  func_unit* funit;              /* Pointer to found functional unit */

  /* Get the valid arguments for this function call */
  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    /* Get the memory information for all covered signals */
    memory_collect( funit, 1, &sig_head, &sig_tail );

    /* Populate covered_memories array */
    sigl = sig_head;
    while( sigl != NULL ) {
      snprintf( tmp, 120, "%d.%d %d.%d %d",
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15), 
                sigl->sig->suppl.part.excluded );
      Tcl_AppendElement( tcl, tmp );
      sigl = sigl->next;
    }

    /* Deallocate list of signals (without deallocating the signals themselves) */
    sig_link_delete_list( sig_head, FALSE );

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

 Populates the global variable "memory_verbose" with the verbose memory coverage information for the
 specified signal in the specified functional unit.
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
  char       str[200];         /* Temporary string for conversion purposes */
  func_unit* funit;            /* Pointer to found functional unit */

  signame = strdup_safe( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    int   str_size;
    char* str;

    memory_get_coverage( funit, signame, &pdim_str, &pdim_array, &udim_str, &memory_info, &excluded );

    str_size = strlen( udim_str ) + 1 + strlen( pdim_str ) + 1 + strlen( pdim_array ) + 1 + strlen( memory_info ) + 1 + 20 + 1;
    str      = Tcl_Alloc( str_size );

    snprintf( str, str_size, "{%s} {%s} {%s} {%s} %d", udim_str, pdim_str, pdim_array, memory_info, excluded );
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
      snprintf( str, 85, "%d.%d %d.%d %d %d", (exprs[i]->line - (startline - 1)), (((exprs[i]->col >> 16) & 0xffff) + 14),
                                              (last->line     - (startline - 1)), ((last->col             & 0xffff) + 15),
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
      snprintf( str, 85, "%d.%d %d.%d %d %d", (exprs[i]->line - (startline - 1)), (((exprs[i]->col >> 16) & 0xffff) + 14),
                                              (last->line     - (startline - 1)), ((last->col             & 0xffff) + 15),
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

  combination_get_expression( expr_id, &code, &uline_groups, &code_size, &ulines, &uline_size, &excludes, &exclude_size );

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
  sig_link*  sig_head;         /* Pointer to head of covered signals */
  sig_link*  sig_tail;         /* Pointer to tail of covered signals */
  sig_link*  sigl;             /* Pointer to current signal link being evaluated */
  char       str[85];          /* Temporary string container */
  int        start_line;       /* Starting line number of this module */
  int*       expr_ids;         /* Array containing the statement IDs of all uncovered FSM signals */
  int*       excludes;         /* Array containing exclude values of all uncovered FSM signals */
  int        i;                /* Loop iterator */
  func_unit* funit;            /* Pointer to found functional unit */

  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    fsm_collect( funit, 0, &sig_head, &sig_tail, &expr_ids, &excludes );

    /* Load uncovered FSMs into Tcl */
    sigl = sig_head;
    i    = 0;
    while( sigl != NULL ) {
      snprintf( str, 85, "%d.%d %d.%d %d %d", (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                                              (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15),
                                              expr_ids[i], excludes[i] );
      Tcl_AppendElement( tcl, str );
      sigl = sigl->next;
      i++;
    }

    /* Deallocate memory */
    sig_link_delete_list( sig_head, FALSE );

    /* If the expr_ids array has one or more elements, deallocate the array */
    if( i > 0 ) {
      free_safe( expr_ids, (sizeof( int ) * i) );
      free_safe( excludes, (sizeof( int ) * i) );
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
    
 Returns the covered FSM expression line/character values for each.
*/  
int tcl_func_collect_covered_fsms(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_COLLECT_COVERED_FSMS);

  int        retval = TCL_OK;  /* Return value for this function */
  sig_link*  sig_head;         /* Pointer to head of covered signals */
  sig_link*  sig_tail;         /* Pointer to tail of covered signals */
  sig_link*  sigl;             /* Pointer to current signal link being evaluated */
  char       str[85];          /* Temporary string container */
  int        start_line;       /* Starting line number of this module */
  int*       expr_ids;         /* Array containing the statement IDs of all uncovered FSM signals */
  int*       excludes;         /* Array containing exclude values of all uncovered FSM signals */
  int        i;                /* Loop iterator */
  func_unit* funit;            /* Pointer to found functional unit */

  start_line = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    fsm_collect( funit, 1, &sig_head, &sig_tail, &expr_ids, &excludes );

    /* Load covered FSMs into Tcl */
    sigl = sig_head;
    i    = 0;
    while( sigl != NULL ) {
      snprintf( str, 85, "%d.%d %d.%d", (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                                        (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15) );
      Tcl_AppendElement( tcl, str );
      sigl = sigl->next;   
      i++;
    }
  
    /* Deallocate memory */
    sig_link_delete_list( sig_head, FALSE );

    /* If the expr_ids array has one or more elements, deallocate the array */
    if( i > 0 ) {
      free_safe( expr_ids, (sizeof( int ) * i) );
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

 Populates the "fsm_states", "fsm_hit_states", "fsm_arcs", "fsm_hit_arcs", "fsm_in_state" and "fsm_out_state"
 global variables with the FSM coverage information from the specified output state expression.
*/
int tcl_func_get_fsm_coverage(
  ClientData  d,      /*!< Not used */
  Tcl_Interp* tcl,    /*!< Pointer to the Tcl interpreter */
  int         argc,   /*!< Number of arguments in the argv list */
  const char* argv[]  /*!< Array of arguments passed to this function */
) { PROFILE(TCL_FUNC_GET_FSM_COVERAGE);

  int          retval = TCL_OK;  /* Return value for this function */
  int          expr_id;          /* Expression ID of output state expression */
  char**       total_states;     /* String array containing all possible states for this FSM */
  unsigned int total_state_num;  /* Number of elements in the total_states array */
  char**       hit_states;       /* String array containing hit states for this FSM */
  unsigned int hit_state_num;    /* Number of elements in the hit_states array */
  char**       total_from_arcs;  /* String array containing all possible state transition input states */
  char**       total_to_arcs;    /* String array containing all possible state transition output states */
  int*         total_ids;        /* Integer array containing exclusion IDs for each state transition */
  int*         excludes;         /* Integer array containing exclude values for each state transition */
  int          total_arc_num;    /* Number of elements in both the total_from_arcs and total_to_arcs arrays */
  char**       hit_from_arcs;    /* String array containing hit state transition input states */
  char**       hit_to_arcs;      /* String array containing hit state transition output states */
  int          hit_arc_num;      /* Number of elements in both the hit_from_arcs and hit_to_arcs arrays */
  char**       input_state;      /* String containing the input state code */
  unsigned int input_size;       /* Number of elements in the input_state array */
  char**       output_state;     /* String containing the output state code */
  unsigned int output_size;      /* Number of elements in the output_state array */
  char         str[4096];        /* Temporary string container */
  int          i;                /* Loop iterator */
  func_unit*   funit;            /* Pointer to found functional unit */

  expr_id = atoi( argv[2] );

  if( (funit = tcl_func_get_funit( tcl, argv[1] )) != NULL ) {

    fsm_get_coverage( funit, expr_id, &total_states, &total_state_num, &hit_states, &hit_state_num,
                      &total_from_arcs, &total_to_arcs, &total_ids, &excludes, &total_arc_num, &hit_from_arcs, &hit_to_arcs, &hit_arc_num,
                      &input_state, &input_size, &output_state, &output_size );

    /* Load FSM total states into Tcl */
    for( i=0; i<total_state_num; i++ ) {
      Tcl_SetVar( tcl, "fsm_states", total_states[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( total_states[i], (strlen( total_states[i] ) + 1) );
    }

    if( total_state_num > 0 ) {
      free_safe( total_states, (sizeof( char* ) * total_state_num) );
    }

    /* Load FSM hit states into Tcl */
    for( i=0; i<hit_state_num; i++ ) {
      Tcl_SetVar( tcl, "fsm_hit_states", hit_states[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( hit_states[i], (strlen( hit_states[i] ) + 1) );
    }

    if( hit_state_num > 0 ) {
      free_safe( hit_states, (sizeof( char* ) * hit_state_num) );
    }

    /* Load FSM total arcs into Tcl */
    for( i=0; i<total_arc_num; i++ ) {
      snprintf( str, 4096, "%s %s %d", total_from_arcs[i], total_to_arcs[i], excludes[i] );
      Tcl_SetVar( tcl, "fsm_arcs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( total_from_arcs[i], (strlen( total_from_arcs[i] ) + 1) );
      free_safe( total_to_arcs[i], (strlen( total_to_arcs[i] ) + 1) );
    }

    if( total_arc_num > 0 ) {
      free_safe( total_from_arcs, (sizeof( char* ) * total_arc_num) );
      free_safe( total_to_arcs, (sizeof( char* ) * total_arc_num) );
      free_safe( total_ids, (sizeof( int ) * total_arc_num) );
      free_safe( excludes, (sizeof( int ) * total_arc_num) );
    }

    /* Load FSM hit arcs into Tcl */
    for( i=0; i<hit_arc_num; i++ ) {
      snprintf( str, 4096, "%s %s", hit_from_arcs[i], hit_to_arcs[i] );
      Tcl_SetVar( tcl, "fsm_hit_arcs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( hit_from_arcs[i], (strlen( hit_from_arcs[i] ) + 1) );
      free_safe( hit_to_arcs[i], (strlen( hit_to_arcs[i] ) + 1) );
    }

    if( hit_arc_num > 0 ) {
      free_safe( hit_from_arcs, (sizeof( char* ) * hit_arc_num) );
      free_safe( hit_to_arcs, (sizeof( char* ) * hit_arc_num) );
    }

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
      snprintf( str, 4096, "{%s} %d %d %d", curr_cp->str, curr_cp->suppl, curr_cp->suppl2, curr_cp->suppl3 );
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
      report_read_cdd_and_ready( ifile, READ_MODE_REPORT_NO_MERGE );
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

    /* Add the specified merge file to the list */
    str_link_add( ifile, &merge_in_head, &merge_in_tail );
    merge_in_num++;

    Try {
      report_read_cdd_and_ready( ifile, READ_MODE_MERGE_INST_MERGE );
    } Catch_anonymous {
      retval = TCL_ERROR;
    }

    free_safe( ifile, (strlen( ifile ) + 1) );

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

  int   retval = TCL_OK;  /* Return value for this function */
  char* ppfilename;       /* Preprocessed filename to return to calling function */
  FILE* out;              /* File handle to preprocessed file */
  int   i;                /* Loop iterator */
  
  /* Add all of the include and define score arguments before calling the preprocessor */
  for( i=0; i<score_arg_num; i++ ) {
    if( strcmp( "-D", score_args[i] ) == 0 ) {
      score_parse_define( score_args[i+1] );
    } else if( strcmp( "-I", score_args[i] ) == 0 ) {
      search_add_include_path( score_args[i+1] );
    }
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
      printf( "tcl_funcs Throw A\n" );
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
  int  i      = 0;       /* Loop iterator */
  
  strcpy( incpath, argv[1] );

  while( !file_exists( incpath ) && (retval == TCL_OK) ) {
    
    /* Find an include path from the score args */
    while( (i < score_arg_num) && (strcmp( "-I", score_args[i] ) != 0) ) i++;
    if( i == score_arg_num ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to find included file \"%s\"", argv[1] );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    } else {
      i++;

      /* Test include score arg for absolute pathname */
      snprintf( incpath, 4096, "%s/%s", score_args[i], argv[1] );

      /* If the absolute pathname is not valid, try a relative pathname */
      if( !file_exists( incpath ) ) {
        snprintf( incpath, 4096, "%s/%s/%s", score_run_path, score_args[i], argv[1] );
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

  int   retval = TCL_OK;    /* Return value for this function */
  char  generation[2];      /* Generation to use for the specified module */
  char* funit_name;         /* Name of functional unit to find generation for */
  int   i;                  /* Loop iterator */
  bool  mod_found = FALSE;  /* Set to TRUE if we found a generation for this exact module */

  funit_name = strdup_safe( argv[1] );
  strcpy( generation, "3" );

  /* Search the entire command-line */
  i = 0;
  while( i < score_arg_num ) {

    /* Find a generation argument in the score args */
    while( (i < score_arg_num) && (strcmp( "-g", score_args[i] ) != 0) ) i++;

    if( i < score_arg_num ) {
      i++;
      if( (strlen( score_args[i] ) == 1) && !mod_found ) {
        generation[0] = score_args[i][(strlen( score_args[i] ) - 1)];
      } else if( ((strlen( score_args[i] ) - 2) == strlen( funit_name )) &&
                 (strncmp( funit_name, score_args[i], strlen( funit_name ) ) == 0) ) {
        generation[0] = score_args[i][(strlen( score_args[i] ) - 1)];
        mod_found     = TRUE;
      }
    
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
      exclude_set_line_exclude( funit, line, value, strdup_safe( reason ), funit->stat );

      /* Now set the line exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->type == funit->type) ) {
          exclude_set_line_exclude( gui_inst_list[i]->funit, line, value, strdup_safe( reason ), gui_inst_list[i]->stat );
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
      exclude_set_line_exclude( inst->funit, line, value, strdup_safe( reason ), inst->stat );

      /* If we are attempting to exclude the line, check all other instances -- if they all exclude this line, exclude the line from the functional unit */
      if( value == 1 ) {

        /* Check the line exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->type != inst->funit->type)       ||
                exclude_is_line_excluded( gui_inst_list[i]->funit, line )) ) i++;

      }

      /* If the line has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->type != inst->funit->type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_line_exclude( gui_funit_list[i], line, value, strdup_safe( reason ), gui_funit_list[i]->stat );
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
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->type == funit->type) ) {
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
                (gui_inst_list[i]->funit->type != inst->funit->type)       ||
                exclude_is_toggle_excluded( gui_inst_list[i]->funit, sig_name )) ) i++;

      }

      /* If the signal has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->type != inst->funit->type)) ) i++;
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
      exclude_set_toggle_exclude( funit, sig_name, value, 'M', strdup_safe( reason ), funit->stat );

      /* Now set the memory exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->type == funit->type) ) {
          exclude_set_toggle_exclude( gui_inst_list[i]->funit, sig_name, value, 'M', strdup_safe( reason ), gui_inst_list[i]->stat );
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
      exclude_set_toggle_exclude( inst->funit, sig_name, value, 'M', strdup_safe( reason ), inst->stat );

      /* If we are attempting to exclude the signal, check all other instances -- if they all exclude this signal, exclude the signal from the functional unit */
      if( value == 1 ) {

        /* Check the toggle exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->type != inst->funit->type)       ||
                exclude_is_toggle_excluded( gui_inst_list[i]->funit, sig_name )) ) i++;

      }

      /* If the signal has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->type != inst->funit->type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_toggle_exclude( gui_funit_list[i], sig_name, value, 'M', strdup_safe( reason ), gui_funit_list[i]->stat );
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
      exclude_set_comb_exclude( funit, expr_id, uline_id, value, strdup_safe( reason ), funit->stat );

      /* Now set the combinational logic exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->type == funit->type) ) {
          exclude_set_comb_exclude( gui_inst_list[i]->funit, expr_id, uline_id, value, strdup_safe( reason ), gui_inst_list[i]->stat ); 
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
      exclude_set_comb_exclude( funit, expr_id, uline_id, value, strdup_safe( reason ), inst->stat );

      /* If we are attempting to exclude the expression, check all other instances -- if they all exclude this expression, exclude the expression from the functional unit */
      if( value == 1 ) {

        /* Check the combinational logic exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->type != inst->funit->type)       ||
                exclude_is_comb_excluded( gui_inst_list[i]->funit, expr_id, uline_id )) ) i++;

      }

      /* If the expression has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->type != inst->funit->type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_comb_exclude( gui_funit_list[i], expr_id, uline_id, value, strdup_safe( reason ), gui_funit_list[i]->stat );
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
      exclude_set_fsm_exclude( funit, expr_id, from_state, to_state, value, strdup_safe( reason ), funit->stat );

      /* Now set the combinational logic exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->type == funit->type) ) {
          exclude_set_fsm_exclude( gui_inst_list[i]->funit, expr_id, from_state, to_state, value, strdup_safe( reason ), gui_inst_list[i]->stat );
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
      exclude_set_fsm_exclude( inst->funit, expr_id, from_state, to_state, value, strdup_safe( reason ), inst->stat );
  
      /* If we are attempting to exclude the expression, check all other instances -- if they all exclude this expression, exclude the expression from the functional unit */
      if( value == 1 ) {
  
        /* Check the combinational logic exclusion for all instances that match our functional unit */
        i = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->type != inst->funit->type)       ||
                exclude_is_fsm_excluded( gui_inst_list[i]->funit, expr_id, from_state, to_state )) ) i++;

      }

      /* If the expression has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->type != inst->funit->type)) ) i++;
        if( i < gui_funit_index ) {
          exclude_set_fsm_exclude( gui_funit_list[i], expr_id, from_state, to_state, value, strdup_safe( reason ), gui_funit_list[i]->stat );
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
      exclude_set_assert_exclude( funit, inst_name, expr_id, value, strdup_safe( reason ), funit->stat );

      /* Now set the combinational logic exclusion in all matching instances in the instance database */
      for( i=0; i<gui_inst_index; i++ ) {
        if( (strcmp( gui_inst_list[i]->funit->name, funit->name ) == 0) && (gui_inst_list[i]->funit->type == funit->type) ) {
          curr_db = 0;
          exclude_set_assert_exclude( gui_inst_list[i]->funit, inst_name, expr_id, value, strdup_safe( reason ), gui_inst_list[i]->stat );
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
      exclude_set_assert_exclude( inst->funit, inst_name, expr_id, value, strdup_safe( reason ), inst->stat );
  
      /* If we are attempting to exclude the expression, check all other instances -- if they all exclude this expression, exclude the expression from the functional unit */
      if( value == 1 ) {
  
        /* Check the combinational logic exclusion for all instances that match our functional unit */
        i       = 0;
        while( (i<gui_inst_index) &&
               ((strcmp( gui_inst_list[i]->funit->name, inst->funit->name ) != 0) ||
                (gui_inst_list[i]->funit->type != inst->funit->type)       ||
                exclude_is_assert_excluded( gui_inst_list[i]->funit, inst_name, expr_id )) ) i++;

      }

      /* If the expression has been excluded from all instances, exclude it from the functional unit database */
      if( i == gui_inst_index ) {
        i = 0;
        while( (i<gui_funit_index) && ((strcmp( gui_funit_list[i]->name, inst->funit->name ) != 0) || (gui_funit_list[i]->type != inst->funit->type)) ) i++;
        if( i < gui_funit_index ) {
          curr_db = 1;
          exclude_set_assert_exclude( gui_funit_list[i], inst_name, expr_id, value, strdup_safe( reason ), gui_funit_list[i]->stat );
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

    report_parse_args( argc, 0, argv );

    assert( output_file != NULL );

    /* Open output stream */
    if( (ofile = fopen( output_file, "w" )) == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open report output file %s for writing", output_file );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      printf( "tcl_funcs Throw B\n" );
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
  const char* browser     /*!< Name of browser executable to use for displaying help information */
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

}
#endif

/*
 $Log$
 Revision 1.83  2008/08/29 05:38:37  phase1geo
 Adding initial pass of FSM exclusion ID output.  Need to fix issues with the -e
 option usage for all metrics, I believe (certainly for FSM).  Checkpointing.

 Revision 1.82  2008/08/23 20:00:30  phase1geo
 Full fix for bug 2054686.  Also cleaned up Cver regressions.

 Revision 1.81  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.77.4.10  2008/08/15 05:11:05  phase1geo
 Converting more old graphics to new style.  Updated documentation.  Cleaned up
 some issues with the build structure per recent documentation changes.  Also fixing
 some issues with the GUI and viewing combination logic coverage that is in an unnamed
 functional unit (more work to do here).  Checkpointing.

 Revision 1.77.4.9  2008/08/08 23:20:28  phase1geo
 Fixing bug with report generator.  Modifying search widgets in output viewer.

 Revision 1.77.4.8  2008/08/07 23:22:49  phase1geo
 Added initial code to synchronize module and instance exclusion information.  Checkpointing.

 Revision 1.77.4.7  2008/08/07 20:51:04  phase1geo
 Fixing memory allocation/deallocation issues with GUI.  Also fixing some issues with FSM
 table output and exclusion.  Checkpointing.

 Revision 1.77.4.6  2008/08/07 18:03:51  phase1geo
 Fixing instance exclusion segfault issue with GUI.  Also cleaned up function
 documentation in link.c.

 Revision 1.77.4.5  2008/08/07 06:39:11  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.77.4.4  2008/08/06 20:11:35  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.77.4.3  2008/07/23 05:10:11  phase1geo
 Adding -d and -ext options to rank and merge commands.  Updated necessary files
 per this change and updated regressions.

 Revision 1.77.4.2  2008/07/19 00:25:52  phase1geo
 Forgot to update some files per the last checkin.

 Revision 1.77.4.1  2008/07/10 22:43:55  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.79  2008/06/28 04:44:22  phase1geo
 More code cleanup.

 Revision 1.78  2008/06/22 05:08:40  phase1geo
 Fixing memory testing error in tcl_funcs.c.  Removed unnecessary output in main_view.tcl.
 Added a few more report diagnostics and started to remove width report files (these will
 no longer be needed and will improve regression runtime and diagnostic memory footprint.

 Revision 1.77  2008/04/15 20:37:11  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.76  2008/04/15 06:08:47  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.75  2008/04/10 23:16:42  phase1geo
 Fixing issues with memory and file handling in preprocessor when an error
 occurs (so that we can recover properly in the GUI).  Also fixing various
 GUI issues from the previous checkin.  Working on debugging problem with
 preprocessing code in verilog.tcl.  Checkpointing.

 Revision 1.74  2008/03/17 22:02:32  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.73  2008/03/17 05:26:17  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.72  2008/03/14 22:00:21  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.71  2008/02/10 03:33:13  phase1geo
 More exception handling added and fixed remaining splint errors.

 Revision 1.70  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.69  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.68  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.67  2007/12/11 23:19:14  phase1geo
 Fixed compile issues and completed first pass injection of profiling calls.
 Working on ordering the calls from most to least.

 Revision 1.66  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.65  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.64  2007/04/11 22:29:49  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.63  2006/10/02 22:41:00  phase1geo
 Lots of bug fixes to memory coverage functionality for GUI.  Memory coverage
 should now be working correctly.  We just need to update the GUI documentation
 as well as other images for the new feature add.

 Revision 1.62  2006/09/27 21:38:35  phase1geo
 Adding code to interract with data in memory coverage verbose window.  Majority
 of code is in place; however, this has not been thoroughly debugged at this point.
 Adding mem3 diagnostic for GUI debugging purposes and checkpointing.

 Revision 1.61  2006/09/26 22:36:38  phase1geo
 Adding code for memory coverage to GUI and related files.  Lots of work to go
 here so we are checkpointing for the moment.

 Revision 1.60  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.59  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.58  2006/08/31 04:02:02  phase1geo
 Adding parsing support for assertions and properties.  Adding feature to
 highlighting support that looks up the generation for the given module and
 highlights accordingly.

 Revision 1.57  2006/07/05 20:43:40  phase1geo
 Finishing Tcl/Tk work on preferences window and report generator window.  Started
 work on GUI HTML documentation for these changes.  A lot of documentation left to
 do here, but all of the missing files are now in place.

 Revision 1.56  2006/07/01 03:16:18  phase1geo
 Completed work on ASCII report generation.  This has not been entirely
 tested out at this time.

 Revision 1.55  2006/06/30 21:05:49  phase1geo
 Updating TODO and adding the ASCII report generation window.  Still some work
 to do here before this completely works.  Right now I am wrestling with the
 window manager to place the components where I want them...

 Revision 1.54  2006/06/29 22:44:57  phase1geo
 Fixing newly introduced bug in FSM report handler.  Also adding pointers back
 to main text window when exclusion properties are changed.  Fixing toggle
 coverage retension.  This is partially working but doesn't seem to want to
 save/restore properly at this point.

 Revision 1.53  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.52  2006/06/29 04:26:02  phase1geo
 More updates for FSM coverage.  We are getting close but are just not to fully
 working order yet.

 Revision 1.51  2006/06/28 22:15:19  phase1geo
 Adding more code to support FSM coverage.  Still a ways to go before this
 is completed.

 Revision 1.50  2006/06/28 04:35:47  phase1geo
 Adding support for line coverage and fixing toggle and combinational coverage
 to redisplay main textbox to reflect exclusion changes.  Also added messageBox
 for close and exit menu options when a CDD has been changed but not saved to
 allow the user to do so before continuing on.

 Revision 1.49  2006/06/27 22:06:26  phase1geo
 Fixing more code related to exclusion.  The detailed combinational expression
 window now works correctly.  I am currently working on getting the main window
 text viewer to display exclusion correctly for all coverage metrics.  Still
 have a ways to go here.

 Revision 1.48  2006/06/26 22:49:00  phase1geo
 More updates for exclusion of combinational logic.  Also updates to properly
 support CDD saving; however, this change causes regression errors, currently.

 Revision 1.47  2006/06/26 04:12:55  phase1geo
 More updates for supporting coverage exclusion.  Still a bit more to go
 before this is working properly.

 Revision 1.46  2006/06/23 19:45:27  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.45  2006/06/23 04:24:03  phase1geo
 Adding functions to tcl_funcs to allow us to change the exclusion property
 for each coverage metric.

 Revision 1.44  2006/06/21 02:56:37  phase1geo
 Fixing assertion error in info.c for merged file writing.

 Revision 1.43  2006/06/20 22:14:32  phase1geo
 Adding support for saving CDD files (needed for file merging and saving exclusion
 information for a CDD file) in the GUI.  Still have a bit to go as I am getting core
 dumps to occur.

 Revision 1.42  2006/06/16 22:44:19  phase1geo
 Beginning to add ability to open/close CDD files without needing to close Covered's
 GUI.  This seems to work but does cause some segfaults yet.

 Revision 1.41  2006/05/03 22:49:42  phase1geo
 Causing all files to be preprocessed when written to the file viewer.  I'm sure that
 I am breaking all kinds of things at this point, but things do work properly on a few
 select test cases so I'm checkpointing here.

 Revision 1.40  2006/05/03 21:17:49  phase1geo
 Finishing assertion source code viewer functionality.  We just need to add documentation
 to the GUI user's guide and we should be set here (though we may want to consider doing
 some syntax highlighting at some point).

 Revision 1.39  2006/05/02 22:06:11  phase1geo
 Fixing problem with passing score path to Tcl from C.

 Revision 1.38  2006/05/02 21:49:41  phase1geo
 Updating regression files -- all but three diagnostics pass (due to known problems).
 Added SCORE_ARGS line type to CDD format which stores the directory that the score
 command was executed from as well as the command-line arguments to the score
 command.

 Revision 1.37  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.36  2006/05/01 13:19:07  phase1geo
 Enhancing the verbose assertion window.  Still need to fix a few bugs and add
 a few more enhancements.

 Revision 1.35  2006/04/29 05:12:14  phase1geo
 Adding initial version of assertion verbose window.  This is currently working; however,
 I think that I want to enhance this window a bit more before calling it good.

 Revision 1.34  2006/04/28 17:10:19  phase1geo
 Adding GUI support for assertion coverage.  Halfway there.

 Revision 1.33  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.32  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.31  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.30  2006/03/27 17:37:23  phase1geo
 Fixing race condition output.  Some regressions may fail due to these changes.

 Revision 1.29  2006/02/08 13:54:23  phase1geo
 Adding previous and next buttons to toggle window.  Added currently selected
 cursor output to main textbox with associated functionality.

 Revision 1.28  2006/02/06 22:48:34  phase1geo
 Several enhancements to GUI look and feel.  Fixed error in combinational logic
 window.

 Revision 1.27  2006/02/03 03:11:15  phase1geo
 Fixing errors in GUI display of combinational logic coverage.  I still see
 a few problems here that need to be taken care of, however.

 Revision 1.26  2006/01/28 06:42:53  phase1geo
 Added configuration read/write functionality for tool preferences and integrated
 the preferences.tcl file into Covered's GUI.  This is now functioning correctly.

 Revision 1.25  2006/01/23 03:53:30  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.24  2006/01/19 23:29:08  phase1geo
 Fixing bug from last checkin in tcl_funcs.c (infinite looping).  Small updates
 to menu.

 Revision 1.23  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.22  2006/01/19 00:01:09  phase1geo
 Lots of changes/additions.  Summary report window work is now complete (with the
 exception of adding extra features).  Added support for parsing left and right
 shift operators and the exponent operator in static expression scenarios.  Fixed
 issues related to GUI (due to recent changes in the score command).  Things seem
 to be generally working as expected with the GUI now.

 Revision 1.21  2005/12/02 05:46:50  phase1geo
 Fixing compile errors when HAVE_TCLTK is defined in config.h.

 Revision 1.20  2005/12/01 19:46:50  phase1geo
 Removed Tcl/Tk from source files if HAVE_TCLTK is not defined.

 Revision 1.19  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.18  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.17  2005/02/07 22:19:46  phase1geo
 Added code to output race condition reasons to informational bar.  Also added code to
 output toggle and combinational logic output to information bar when cursor is over
 an expression that, when clicked on, will take you to the detailed coverage window.

 Revision 1.16  2005/02/05 05:29:25  phase1geo
 Added race condition reporting to GUI.

 Revision 1.15  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.14  2004/09/14 19:26:28  phase1geo
 Fixing browser and version injection to Tcl scripts.

 Revision 1.13  2004/09/14 04:54:58  phase1geo
 Adding check for browser to configuration build scripts.  Adding code to set
 BROWSER global variable in Tcl scripts.

 Revision 1.12  2004/08/17 15:23:37  phase1geo
 Added combinational logic coverage output to GUI.  Modified comb.c code to get this
 to work that impacts ASCII coverage output; however, regression is fully passing with
 these changes.  Combinational coverage for GUI is mostly complete regarding information
 and usability.  Possibly some cleanup in output and in code is needed.

 Revision 1.11  2004/08/13 20:45:05  phase1geo
 More added for combinational logic verbose reporting.  At this point, the
 code is being output with underlines that can move up and down the expression
 tree.  No expression reporting is being done at this time, however.

 Revision 1.10  2004/08/12 12:56:32  phase1geo
 Fixing error in combinational logic collection function for covered logic.

 Revision 1.9  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.8  2004/08/10 17:23:58  phase1geo
 Fixing various user-related problems with interface.  Things are working pretty
 well at this point, I believe.

 Revision 1.7  2004/08/10 15:58:13  phase1geo
 Fixing problems with toggle coverage when modules start on lines above 1 and
 problems when signal is a single or multi-bit select.

 Revision 1.6  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.5  2004/04/21 05:14:03  phase1geo
 Adding report_gui checking to print_output and adding error handler to TCL
 scripts.  Any errors occurring within the code will be propagated to the user.

 Revision 1.4  2004/04/17 14:07:55  phase1geo
 Adding replace and merge options to file menu.

 Revision 1.3  2004/03/26 13:20:33  phase1geo
 Fixing case where user hits cancel button in open window so that we don't
 exit the GUI when this occurs.

 Revision 1.2  2004/03/25 14:37:07  phase1geo
 Fixing installation of TCL scripts and usage of the scripts in their installed
 location.  We are almost ready to create the new development release.

 Revision 1.1  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

*/
