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
 \author   Trevor Williams  (trevorw@charter.net)
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

#include "defines.h"
#include "tcl_funcs.h"
#include "gui.h"
#include "link.h"
#include "util.h"
#include "line.h"
#include "toggle.h"
#include "comb.h"
#include "expr.h"
#include "instance.h"
#include "report.h"
#include "race.h"
#include "fsm.h"
#include "assertion.h"
#include "search.h"
#include "score.h"


extern funit_link* funit_head;
extern funit_inst* instance_root;
extern char        user_msg[USER_MSG_LENGTH];
extern const char* race_msgs[RACE_TYPE_NUM];
extern char        score_run_path[4096];
extern char**      score_args;
extern int         score_arg_num;
extern void        reset_pplexer( const char* filename, FILE* out );
extern int         PPVLlex( void );
extern char**      merge_in;
extern int         merge_in_num;

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Retrieves all of the race condition messages for all possible race conditions and stores them into
 the "race_msgs" global array.
*/
int tcl_func_get_race_reason_msgs( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] ) {

  int retval = TCL_OK;  /* Return value of this function */
  int i;                /* Loop iterator */

  for( i=0; i<RACE_TYPE_NUM; i++ ) {
    strcpy( user_msg, race_msgs[i] );
    Tcl_SetVar( tcl, "race_msgs", user_msg, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "funit_names" and "funit_types" with all of the functional units from the
 design.
*/
int tcl_func_get_funit_list( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] ) {

  char** funit_names;      /* List of functional unit names in design */
  char** funit_types;      /* List of functional unit types in design */
  int    funit_size;       /* Number of elements in funit_list */
  int    retval = TCL_OK;  /* Return value of this function  */
  int    i;                /* Loop iterator */

  if( funit_get_list( &funit_names, &funit_types, &funit_size ) ) {
    for( i=0; i<funit_size; i++ ) {
      Tcl_SetVar( tcl, "funit_names", funit_names[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      Tcl_SetVar( tcl, "funit_types", funit_types[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to get functional unit list from this design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \param tcl   Pointer to the Tcl interpreter
 \param root  Pointer to current root instance to output

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "inst_list", "funit_names", and "funit_types" with all of the instances
 from the design.
*/
int tcl_func_get_instances( Tcl_Interp* tcl, funit_inst* root ) {

  funit_inst* curr;         /* Pointer to current functional unit instance */
  char        scope[4096];  /* Hierarchical scope name */
  char        tmpstr[10];   /* Temporary string */

  /* Generate the name of this child */
  scope[0] = '\0';
  instance_gen_scope( scope, root );
  Tcl_SetVar( tcl, "inst_list",   scope,             (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
  Tcl_SetVar( tcl, "funit_names", root->funit->name, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
  snprintf( tmpstr, 10, "%d", root->funit->type );
  Tcl_SetVar( tcl, "funit_types", tmpstr,            (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );

  curr = root->child_head;
  while( curr != NULL ) {
    tcl_func_get_instances( tcl, curr );
    curr = curr->next;
  }

  return( TCL_OK );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "inst_list", "funit_names", and "funit_types" with all of the instances 
 from the design.
*/
int tcl_func_get_instance_list( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int retval = TCL_OK;  /* Return value for this function */

  if( instance_root != NULL ) {
    tcl_func_get_instances( tcl, instance_root );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to get instance list from this design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Gets the filename for the specified functional unit name and type and places this value in the "file_name"
 global variable.
*/
int tcl_func_get_filename( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* filename;         /* Name of file containing the specified functional unit */

  if( (filename = funit_get_filename( argv[1], atoi( argv[2] ) )) != NULL ) {
    Tcl_SetVar( tcl, "file_name", filename, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find filename for functional unit %s", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "start_line" and "end_line" with the starting and ending line numbers of the
 specified functional unit within its file.
*/
int tcl_func_get_funit_start_and_end( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int  retval = TCL_OK;  /* Return value for this function */
  int  start_line;       /* Starting line number of the given functional unit */
  int  end_line;         /* Ending line number of the given functional unit */
  char linenum[20];      /* Temporary string container */

  if( funit_get_start_and_end_lines( argv[1], atoi( argv[2] ), &start_line, &end_line ) ) {
    snprintf( linenum, 20, "%d", start_line );
    Tcl_SetVar( tcl, "start_line", linenum, TCL_GLOBAL_ONLY );
    snprintf( linenum, 20, "%d", end_line );
    Tcl_SetVar( tcl, "end_line",   linenum, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find start and end lines for functional unit %s", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variable "uncovered_lines" with the line numbers of all lines that were found to be uncovered
 during simulation.
*/
int tcl_func_collect_uncovered_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit to get uncovered lines for */
  int   funit_type;       /* Type of functional unit to get uncovered lines for */
  int*  lines;            /* Array of line numbers that were found to be uncovered during simulation */
  int   line_cnt;         /* Number of elements in the lines array */
  int   i;                /* Loop iterator */
  char  line[20];         /* Temporary string container */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( line_collect( funit_name, funit_type, 0, &lines, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line, 20, "%d", lines[i] );
      Tcl_SetVar( tcl, "uncovered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }

    free_safe( lines );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variable "covered_lines" with the line numbers of all lines that were found to be covered
 during simulation.
*/
int tcl_func_collect_covered_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval  = TCL_OK;  /* Return value for this function */
  char* funit_name;        /* Name of functional unit to get covered line information for */
  int   funit_type;        /* Type of functional unit to get covered line information for */
  int*  lines;             /* Array of line numbers that were covered during simulation */
  int   line_cnt;          /* Number of elements in the lines array */
  int   i;                 /* Loop iterator */
  char  line[20];          /* Temporary string container */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( line_collect( funit_name, funit_type, 1, &lines, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line, 20, "%d", lines[i] );
      Tcl_SetVar( tcl, "covered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }

    free_safe( lines );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "race_lines" and "race_reasons" with the race condition information for
 the specified functional unit.
*/
int tcl_func_collect_race_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit to get race condition information for */
  int   funit_type;       /* Type of functional unit to get race condition information for */
  int   start_line;       /* Starting line of specified functional unit */
  int*  slines;           /* Starting line numbers of statement blocks containing race condition(s) */
  int*  elines;           /* Ending line numbers of statement blocks containing race conditions(s) */
  int*  reasons;          /* Reason for race condition for a specified statement block */
  int   line_cnt;         /* Number of valid entries in the slines, elines and reasons arrays */
  int   i      = 0;       /* Loop iterator */
  char  line[50];         /* Temporary string containing line information */
  char  reason[20];       /* Temporary string containing reason information */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  start_line = atoi( argv[3] );

  if( race_collect_lines( funit_name, funit_type, &slines, &elines, &reasons, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line,   50, "%d.0 %d.end", (slines[i] - (start_line - 1)), (elines[i] - (start_line - 1)) );
      snprintf( reason, 20, "%d", reasons[i] );
      Tcl_SetVar( tcl, "race_lines",   line,   (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      Tcl_SetVar( tcl, "race_reasons", reason, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }

    free_safe( slines );
    free_safe( elines );
    free_safe( reasons );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[i] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variable "uncovered_toggles" with the names of all signals of the given functional unit
 that did not achieve 100% toggle coverage.
*/
int tcl_func_collect_uncovered_toggles( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int       retval   = TCL_OK;  /* Return value for this function */
  char*     funit_name;         /* Functional unit name to get uncovered signal names for */
  int       funit_type;         /* Functional unit type to get uncovered signal names for */
  sig_link* sig_head = NULL;    /* Pointer to head of signal list */
  sig_link* sig_tail = NULL;    /* Pointer to tail of signal list */
  sig_link* sigl;               /* Pointer to current signal link being evaluated */
  char      tmp[85];            /* Temporary string */
  int       start_line;         /* Starting line number */

  /* Get the valid arguments for this command */
  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  start_line = atoi( argv[3] );

  /* Find all signals that did not achieve 100% coverage */
  if( toggle_collect( funit_name, funit_type, 0, &sig_head, &sig_tail ) ) {

    sigl = sig_head;
    while( sigl != NULL ) {
      snprintf( tmp, 85, "%d.%d %d.%d",
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15) );
      Tcl_SetVar( tcl, "uncovered_toggles", tmp, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      sigl = sigl->next;
    }

    /* Deallocate signal list (without destroying signals) */
    sig_link_delete_list( sig_head, FALSE );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function
 
 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variable "covered_toggles" with the names of all signals of the given functional unit
 that achieved 100% toggle coverage.
*/
int tcl_func_collect_covered_toggles( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int       retval   = TCL_OK;  /* Return value for this function */
  char*     funit_name;         /* Functional unit name to find */
  int       funit_type;         /* Functional unit type to find */
  sig_link* sig_head = NULL;    /* Pointer to head of signal list */
  sig_link* sig_tail = NULL;    /* Pointer to tail of signal list */
  sig_link* sigl;               /* Pointer to current signal being evaluated */
  char      tmp[85];            /* Temporary string */
  int       start_line;         /* Starting line number of this functional unit */

  /* Get the valid arguments for this function call */
  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  start_line = atoi( argv[3] );

  /* Get the toggle information for all covered signals */
  if( toggle_collect( funit_name, funit_type, 1, &sig_head, &sig_tail ) ) {

    sigl = sig_head;
    while( sigl != NULL ) {
      snprintf( tmp, 85, "%d.%d %d.%d",
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15) );
      Tcl_SetVar( tcl, "covered_toggles", tmp, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      sigl = sigl->next;
    }

    /* Deallocate list of signals (without deallocating the signals themselves) */
    sig_link_delete_list( sig_head, FALSE );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "toggle_msb", "toggle_lsb", "toggle01_verbose", and "toggle10_verbose" with
 the verbose coverage information for the specified signal in the specified functional unit.
*/
int tcl_func_get_toggle_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit containing the signal to get verbose toggle information for */
  int   funit_type;       /* Type of functional unit containing the signal to get verbose toggle information for */
  char* signame;          /* Name of signal to get verbose toggle information for */
  int   msb;              /* Most-significant bit position of the specified signal */
  int   lsb;              /* Least-significant bit position of the specified signal */
  char* tog01;            /* Toggle 0->1 information for this signal */
  char* tog10;            /* Toggle 1->0 information for this signal */
  char  tmp[20];          /* Temporary string for conversion purposes */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  signame    = strdup_safe( argv[3], __FILE__, __LINE__ );

  if( toggle_get_coverage( funit_name, funit_type, signame, &msb, &lsb, &tog01, &tog10 ) ) {

    snprintf( tmp, 20, "%d", msb );
    Tcl_SetVar( tcl, "toggle_msb", tmp, TCL_GLOBAL_ONLY );
    snprintf( tmp, 20, "%d", lsb );
    Tcl_SetVar( tcl, "toggle_lsb", tmp, TCL_GLOBAL_ONLY );
    Tcl_SetVar( tcl, "toggle01_verbose", tog01, TCL_GLOBAL_ONLY );
    Tcl_SetVar( tcl, "toggle10_verbose", tog10, TCL_GLOBAL_ONLY );

    /* Free up allocated memory */
    free_safe( tog01 );
    free_safe( tog10 );

  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free up allocated memory */
  free_safe( funit_name );
  free_safe( signame );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "uncovered_combs" and "covered_combs" with the uncovered and covered combinational
 expression line/character values for each.
*/
int tcl_func_collect_combs( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int          retval = TCL_OK;  /* Return value for this function */
  char*        funit_name;       /* Name of functional unit to get combinational logic coverage info for */
  int          funit_type;       /* Type of functional unit to get combinational logic coverage info for */
  expression** covs;             /* Array of expression pointers to fully covered expressions */
  expression** uncovs;           /* Array of expression pointers to uncovered expressions */
  int          cov_cnt;          /* Number of elements in the covs array */
  int          uncov_cnt;        /* Number of elements in the uncovs array */
  int          i;                /* Loop iterator */
  char         str[85];          /* Temporary string container */
  int          startline;        /* Starting line number of this module */
  expression*  last;             /* Pointer to expression in an expression tree that is on the last line */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  startline  = atoi( argv[3] );

  if( combination_collect( funit_name, funit_type, &covs, &cov_cnt, &uncovs, &uncov_cnt ) ) {

    /* Load uncovered statements into Tcl */
    for( i=0; i<uncov_cnt; i++ ) {
      last = expression_get_last_line_expr( uncovs[i] );
      snprintf( str, 85, "%d.%d %d.%d %d", (uncovs[i]->line - (startline - 1)), (((uncovs[i]->col >> 16) & 0xffff) + 14),
                                           (last->line      - (startline - 1)), ((last->col              & 0xffff) + 15),
                                           uncovs[i]->id );
      Tcl_SetVar( tcl, "uncovered_combs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }

    /* Load covered statements into Tcl */
    for( i=0; i<cov_cnt; i++ ) {
      last = expression_get_last_line_expr( covs[i] );
      snprintf( str, 85, "%d.%d %d.%d", (covs[i]->line - (startline - 1)), (((covs[i]->col >> 16) & 0xffff) + 14),
                                        (last->line    - (startline - 1)), ((last->col            & 0xffff) + 15) );
      Tcl_SetVar( tcl, "covered_combs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }

    /* Deallocate memory */
    free_safe( uncovs );
    free_safe( covs   );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Retrieves the verbose combination expression information for a given expression, populating the "comb_code",
 "comb_uline_groups" and "comb_ulines" global variables with the code and underline information.
*/
int tcl_func_get_comb_expression( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int    retval = TCL_OK;  /* Return value for this function */
  char*  funit_name;       /* Name of functional unit containing expression to find */
  int    funit_type;       /* Type of functional unit containing expression to find */
  int    expr_id;          /* Expression ID of expression to find within the given functional unit */
  char** code;             /* Array of strings containing the combinational logic code returned from the code generator */
  int*   uline_groups;     /* Array of integers representing the number of underline lines found under each line of code */
  int    code_size;        /* Number of elements stored in the code array */
  char** ulines;           /* Array of strings containing the underline lines returned from the underliner */
  int    uline_size;       /* Number of elements stored in the ulines array */
  int    i;                /* Loop iterator */
  char   tmp[20];          /* Temporary string container */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  expr_id    = atoi( argv[3] );

  if( combination_get_expression( funit_name, funit_type, expr_id, &code, &uline_groups, &code_size, &ulines, &uline_size ) ) {

    for( i=0; i<code_size; i++ ) {
      Tcl_SetVar( tcl, "comb_code", code[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      snprintf( tmp, 20, "%d", uline_groups[i] );
      Tcl_SetVar( tcl, "comb_uline_groups", tmp, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( code[i] );
    }

    for( i=0; i<uline_size; i++ ) {
      Tcl_SetVar( tcl, "comb_ulines", ulines[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( ulines[i] );
    }

    /* Free up allocated memory */
    if( code_size > 0 ) {
      free_safe( code );
      free_safe( uline_groups );
    }

    if( uline_size > 0 ) {
      free_safe( ulines );
    }

  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free up allocated memory */
  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "comb_expr_cov" global variable with the coverage information for the specified
 subexpression with the given underline identifier.
*/
int tcl_func_get_comb_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int    retval = TCL_OK;  /* Return value for this function */
  char*  funit_name;       /* Name of functional unit containing expression to lookup */
  int    funit_type;       /* Type of functional unit containing expression to lookup */
  int    expid;            /* Expression ID of statement containing desired subexpression */
  int    ulid;             /* Underline ID of expression to find */
  char** info;             /* Array containing lines of coverage information text */
  int    info_size;        /* Specifies number of elements in info array */
  int    i;                /* Loop iterator */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  expid      = atoi( argv[3] );
  ulid       = atoi( argv[4] );

  if( combination_get_coverage( funit_name, funit_type, expid, ulid, &info, &info_size ) ) {

    if( info_size > 0 ) {

      for( i=0; i<info_size; i++ ) {
        Tcl_SetVar( tcl, "comb_expr_cov", info[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
        free_safe( info[i] );
      }

      free_safe( info );

    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s and/or expression ID %d in design", argv[1], ulid );
    Tcl_AddErrorInfo( tcl, user_msg );
    retval = TCL_ERROR;
  }

  /* Free up allocated memory */
  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "uncovered_fsms" and "covered_fsms" with the uncovered and covered FSM
 expression line/character values for each.
*/
int tcl_func_collect_fsms( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int          retval = TCL_OK;  /* Return value for this function */
  char*        funit_name;       /* Name of functional unit to get combinational logic coverage info for */
  int          funit_type;       /* Type of functional unit to get combinational logic coverage info for */
  sig_link*    cov_head;         /* Pointer to head of covered signals */
  sig_link*    cov_tail;         /* Pointer to tail of covered signals */
  sig_link*    uncov_head;       /* Pointer to head of uncovered signals */
  sig_link*    uncov_tail;       /* Pointer to tail of uncovered signals */
  sig_link*    sigl;             /* Pointer to current signal link being evaluated */
  char         str[85];          /* Temporary string container */
  int          start_line;       /* Starting line number of this module */
  int*         expr_ids;         /* Array containing the statement IDs of all uncovered FSM signals */
  int          i;                /* Loop iterator */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  start_line = atoi( argv[3] );

  if( fsm_collect( funit_name, funit_type, &cov_head, &cov_tail, &uncov_head, &uncov_tail, &expr_ids ) ) {

    /* Load uncovered FSMs into Tcl */
    sigl = uncov_head;
    i    = 0;
    while( sigl != NULL ) {
      snprintf( str, 85, "%d.%d %d.%d %d", (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                                           (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15),
                                           expr_ids[i] );
      Tcl_SetVar( tcl, "uncovered_fsms", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      sigl = sigl->next;
      i++;
    }

    /* Load covered FSMs into Tcl */
    sigl = cov_head;
    while( sigl != NULL ) {
      snprintf( str, 85, "%d.%d %d.%d", (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 14),
                                        (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 15) );
      Tcl_SetVar( tcl, "covered_fsms", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      sigl = sigl->next;
    }

    /* Deallocate memory */
    sig_link_delete_list( cov_head,   FALSE );
    sig_link_delete_list( uncov_head, FALSE );

    /* If the expr_ids array has one or more elements, deallocate the array */
    if( i > 0 ) {
      free_safe( expr_ids );
    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "fsm_states", "fsm_hit_states", "fsm_arcs", "fsm_hit_arcs", "fsm_in_state" and "fsm_out_state"
 global variables with the FSM coverage information from the specified output state expression.
*/
int tcl_func_get_fsm_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int          retval = TCL_OK;  /* Return value for this function */
  char*        funit_name;       /* Name of functional unit to get combinational logic coverage info for */
  int          funit_type;       /* Type of functional unit to get combinational logic coverage info for */
  int          expr_id;          /* Expression ID of output state expression */
  int          width;            /* Width of output state expression */
  char**       total_states;     /* String array containing all possible states for this FSM */
  int          total_state_num;  /* Number of elements in the total_states array */
  char**       hit_states;       /* String array containing hit states for this FSM */
  int          hit_state_num;    /* Number of elements in the hit_states array */
  char**       total_from_arcs;  /* String array containing all possible state transition input states */
  char**       total_to_arcs;    /* String array containing all possible state transition output states */
  int          total_arc_num;    /* Number of elements in both the total_from_arcs and total_to_arcs arrays */
  char**       hit_from_arcs;    /* String array containing hit state transition input states */
  char**       hit_to_arcs;      /* String array containing hit state transition output states */
  int          hit_arc_num;      /* Number of elements in both the hit_from_arcs and hit_to_arcs arrays */
  char**       input_state;      /* String containing the input state code */
  int          input_size;       /* Number of elements in the input_state array */
  char**       output_state;     /* String containing the output state code */
  int          output_size;      /* Number of elements in the output_state array */
  char         str[4096];        /* Temporary string container */
  int          i;                /* Loop iterator */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  expr_id    = atoi( argv[3] );

  if( fsm_get_coverage( funit_name, funit_type, expr_id, &width, &total_states, &total_state_num, &hit_states, &hit_state_num,
                        &total_from_arcs, &total_to_arcs, &total_arc_num, &hit_from_arcs, &hit_to_arcs, &hit_arc_num,
                        &input_state, &input_size, &output_state, &output_size ) ) {

    /* Load FSM total states into Tcl */
    for( i=0; i<total_state_num; i++ ) {
      snprintf( str, 4096, "%d'h%s", width, total_states[i] );
      Tcl_SetVar( tcl, "fsm_states", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( total_states[i] );
    }

    if( total_state_num > 0 ) {
      free_safe( total_states );
    }

    /* Load FSM hit states into Tcl */
    for( i=0; i<hit_state_num; i++ ) {
      snprintf( str, 4096, "%d'h%s", width, hit_states[i] );
      Tcl_SetVar( tcl, "fsm_hit_states", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( hit_states[i] );
    }

    if( hit_state_num > 0 ) {
      free_safe( hit_states );
    }

    /* Load FSM total arcs into Tcl */
    for( i=0; i<total_arc_num; i++ ) {
      snprintf( str, 4096, "%d'h%s %d'h%s", width, total_from_arcs[i], width, total_to_arcs[i] );
      Tcl_SetVar( tcl, "fsm_arcs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( total_from_arcs[i] );
      free_safe( total_to_arcs[i] );
    }

    if( total_arc_num > 0 ) {
      free_safe( total_from_arcs );
      free_safe( total_to_arcs );
    }

    /* Load FSM hit arcs into Tcl */
    for( i=0; i<hit_arc_num; i++ ) {
      snprintf( str, 4096, "%d'h%s %d'h%s", width, hit_from_arcs[i], width, hit_to_arcs[i] );
      Tcl_SetVar( tcl, "fsm_hit_arcs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( hit_from_arcs[i] );
      free_safe( hit_to_arcs[i] );
    }

    if( hit_arc_num > 0 ) {
      free_safe( hit_from_arcs );
      free_safe( hit_to_arcs );
    }

    /* Load FSM input state into Tcl */
    if( input_size > 0 ) {
      Tcl_SetVar( tcl, "fsm_in_state", input_state[0], TCL_GLOBAL_ONLY );
      for( i=0; i<input_size; i++ ) {
        free_safe( input_state[i] );
      }
      free_safe( input_state );
    }

    /* Load FSM output state into Tcl */
    if( output_size > 0 ) {
      Tcl_SetVar( tcl, "fsm_out_state", output_state[0], TCL_GLOBAL_ONLY );
      for( i=0; i<output_size; i++ ) {
        free_safe( output_state[i] );
      }
      free_safe( output_state );
    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "uncovered_asserts" and "covered_asserts" with the uncovered and covered assertion
 module instance names.
*/
int tcl_func_collect_assertions( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int    retval = TCL_OK;   /* Return value for this function */
  char*  funit_name;        /* Name of functional unit to get combinational logic coverage info for */
  int    funit_type;        /* Type of functional unit to get combinational logic coverage info for */
  char** uncov_inst_names;  /* Array of instance names for all uncovered assertions in the specified functional unit */
  int    uncov_inst_size;   /* Number of valid elements in the uncov_inst_names array */
  char** cov_inst_names;    /* Array of instance names for all covered assertions in the specified functional unit */
  int    cov_inst_size;     /* Number of valid elements in the cov_inst_names array */
  int    i;                 /* Loop iterator */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( assertion_collect( funit_name, funit_type, &uncov_inst_names, &uncov_inst_size, &cov_inst_names, &cov_inst_size ) ) {

    /* Load uncovered assertions into Tcl */
    for( i=0; i<uncov_inst_size; i++ ) {
      Tcl_SetVar( tcl, "uncovered_asserts", uncov_inst_names[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( uncov_inst_names[i] );
    }

    if( uncov_inst_size > 0 ) {
      free_safe( uncov_inst_names );
    }

    /* Load covered assertions into Tcl */
    for( i=0; i<cov_inst_size; i++ ) {
      Tcl_SetVar( tcl, "covered_asserts", cov_inst_names[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      free_safe( cov_inst_names[i] );
    }
    
    if( cov_inst_size > 0 ) {
      free_safe( cov_inst_names );
    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( funit_name );
  
  return( retval );
  
}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "assert_cov_mod" and "assert_cov_points" global variables with the coverage points from the
 given instance.
*/
int tcl_func_get_assert_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int       retval = TCL_OK;  /* Return value for this function */
  char*     funit_name;       /* Name of functional unit to find */
  int       funit_type;       /* Type of functional unit to find */
  char*     inst_name;        /* Name of assertion module instance to get coverage information for */
  char*     assert_mod;       /* Name of assertion module for the given instance name */
  str_link* cp_head;          /* Pointer to head of coverage point list */
  str_link* cp_tail;          /* Pointer to tail of coverage point list */
  str_link* curr_cp;          /* Pointer to current coverage point to write */
  char      str[4096];        /* Temporary string holder */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  inst_name  = strdup_safe( argv[3], __FILE__, __LINE__ );

  if( assertion_get_coverage( funit_name, funit_type, inst_name, &assert_mod, &cp_head, &cp_tail ) ) {

    Tcl_SetVar( tcl, "assert_cov_mod", assert_mod, TCL_GLOBAL_ONLY );
    free_safe( assert_mod );

    curr_cp = cp_head;
    while( curr_cp != NULL ) {
      snprintf( str, 4096, "{%s} %d", curr_cp->str, curr_cp->suppl );
      Tcl_SetVar( tcl, "assert_cov_points", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      curr_cp = curr_cp->next;
    }

    /* Deallocate the string list */
    str_link_delete_list( cp_head );

  }

  free_safe( funit_name );
  free_safe( inst_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Opens the specified CDD file, reading its contents into the database.
*/
int tcl_func_open_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* ifile;            /* Name of CDD file to open */

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1], __FILE__, __LINE__ );

    if( !report_read_cdd_and_ready( ifile, READ_MODE_REPORT_MOD_MERGE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open CDD \"%s\"", ifile );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    }

    free_safe( ifile );

  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Closes the current CDD file, freeing all associated memory with it.
*/
int tcl_func_close_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int retval = TCL_OK;  /* Return value for this function */

  if( !report_close_cdd() ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to close CDD file" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Saves the current CDD file with the specified name.
*/
int tcl_func_save_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* filename;         /* Name of file to save as */

  printf( "Saving CDD file %s\n", argv[1] );

  filename = strdup_safe( argv[1], __FILE__, __LINE__ );

  if( !report_save_cdd( filename ) ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to save CDD file \"%s\"", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( filename );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Replaces the current CDD database with the contents of the given CDD filename.  The CDD file replacing
 the current CDD file must be generated from the same design or an error will occur.
*/
int tcl_func_replace_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* ifile;            /* Name of CDD file to replace the existing file */

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1], __FILE__, __LINE__ );

    if( !report_read_cdd_and_ready( ifile, READ_MODE_REPORT_MOD_REPLACE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to replace current CDD with \"%s\"", ifile );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    }

    free_safe( ifile );

  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Merges the specified CDD file with the current CDD database.
*/
int tcl_func_merge_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* ifile;            /* Name of CDD file to merge */

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1], __FILE__, __LINE__ );

    /* Add the specified merge file to the list */
    merge_in               = (char**)realloc( merge_in, (sizeof( char* ) * (merge_in_num + 1)) );
    merge_in[merge_in_num] = ifile;
    merge_in_num++;

    if( !report_read_cdd_and_ready( ifile, READ_MODE_REPORT_MOD_MERGE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to merge current CDD with \"%s\"", ifile );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    }

  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the "line_summary_total" and "line_summary_hit" global variables with the total number of lines
 and total number of lines hit during simulation information for the specified functional unit.
*/
int tcl_func_get_line_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit to lookup */
  int   funit_type;       /* Type of functional unit to lookup */
  int   total;            /* Contains total number of lines evaluated */
  int   hit;              /* Contains total number of lines hit */
  char  value[20];        /* String version of a value */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
 
  if( line_get_funit_summary( funit_name, funit_type, &total, &hit ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "line_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "line_summary_hit", value, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "toggle_summary_total" and "toggle_summary_hit" to the total number
 of signals evaluated for toggle coverage and the total number of signals with complete toggle coverage
 for the specified functional unit.
*/
int tcl_func_get_toggle_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit to lookup */
  int   funit_type;       /* Type of functional unit to lookup */
  int   total;            /* Contains total number of signals evaluated */
  int   hit;              /* Contains total number of signals hit */
  char  value[20];        /* String version of a value */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
		     
  if( toggle_get_funit_summary( funit_name, funit_type, &total, &hit ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "toggle_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "toggle_summary_hit", value, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "comb_summary_total" and "comb_summary_hit" to the total number
 of expression values evaluated for combinational logic coverage and the total number of expression values
 with complete combinational logic coverage for the specified functional unit.
*/
int tcl_func_get_comb_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit to lookup */
  int   funit_type;       /* Type of functional unit to lookup */
  int   total;            /* Contains total number of expressions evaluated */
  int   hit;              /* Contains total number of expressions hit */
  char  value[20];        /* String version of a value */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( combination_get_funit_summary( funit_name, funit_type, &total, &hit ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "comb_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "comb_summary_hit", value, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "fsm_summary_total" and "fsm_summary_hit" to the total number
 of state transitions evaluated for FSM coverage and the total number of state transitions
 with complete FSM state transition coverage for the specified functional unit.
*/
int tcl_func_get_fsm_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit to lookup */
  int   funit_type;       /* Type of functional unit to lookup */
  int   total;            /* Contains total number of expressions evaluated */
  int   hit;              /* Contains total number of expressions hit */
  char  value[20];        /* String version of a value */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( fsm_get_funit_summary( funit_name, funit_type, &total, &hit ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "fsm_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "fsm_summary_hit", value, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Populates the global variables "assert_summary_total" and "assert_summary_hit" to the total number
 of assertions evaluated for coverage and the total number of hit assertions for the specified functional unit.
*/
int tcl_func_get_assert_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of functional unit to lookup */
  int   funit_type;       /* Type of functional unit to lookup */
  int   total;            /* Contains total number of expressions evaluated */
  int   hit;              /* Contains total number of expressions hit */
  char  value[20];        /* String version of a value */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( assertion_get_funit_summary( funit_name, funit_type, &total, &hit ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "assert_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "assert_summary_hit", value, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Preprocesses the specified filename, outputting the contents into a temporary file whose name is passed back
 to the calling function.
*/
int tcl_func_preprocess_verilog( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* ppfilename;       /* Preprocessed filename to return to calling function */
  FILE* out;              /* File handle to preprocessed file */
  int   i;                /* Loop iterator */
  
  /* Add all of the include and define score arguments before calling the preprocessor */
  for( i=0; i<score_arg_num; i++ ) {
    if( strcmp( "-D", score_args[i] ) == 0 ) {
      score_parse_define( score_args[i+1] );
    } else if( strcmp( "-I", score_args[i] ) == 0 ) {
      search_add_include_path( score_args[i] );
    }
  }

  /* Create temporary output filename */
  ppfilename = Tcl_Alloc( 10 );
  snprintf( ppfilename, 10, "tmpXXXXXX" );
  assert( mkstemp( ppfilename ) != 0 );

  out = fopen( ppfilename, "w" );
  if( out == NULL ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to open temporary file %s for writing", ppfilename );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

  /* Now the preprocessor on this file first */
  reset_pplexer( argv[1], out );
  PPVLlex();

  fclose( out );
  
  /* Set the return value to the name of the temporary file */
  Tcl_SetResult( tcl, ppfilename, TCL_DYNAMIC );

  return( retval );
  
}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the score directory pathname to the calling Tcl process.
*/
int tcl_func_get_score_path( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int retval = TCL_OK;  /* Return value for this function */

  Tcl_SetResult( tcl, score_run_path, TCL_STATIC );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Returns the full pathname of the specified included file.  Uses the -I options specified in the CDD file
 for reference.
*/
int tcl_func_get_include_pathname( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int  retval = TCL_OK;  /* Return value for this function */
  char incpath[4096];    /* Contains full included pathname */
  int  i      = 0;       /* Loop iterator */
  
  strcpy( incpath, argv[1] );

  while( !file_exists( incpath ) && (retval == TCL_OK) ) {
    
    /* Find an include path from the score args */
    while( (i < score_arg_num) && (strcmp( "-I", score_args[i] ) != 0) ) i++;
    if( i == score_arg_num ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to find included file" );
      Tcl_AddErrorInfo( tcl, user_msg );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    } else {
      i++;
      snprintf( incpath, 4096, "%s/%s", score_args[i], argv[1] );
    }

  }

  if( retval == TCL_OK ) {
    Tcl_SetResult( tcl, incpath, TCL_STATIC );
  }

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_line_exclude( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of current functional unit */
  int   funit_type;       /* Type of current functional unit */
  int   line;             /* Line number that is being set */
  int   value;            /* Value to set the exclusion value to */

  /* Get argument values */
  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  line       = atoi( argv[3] );
  value      = atoi( argv[4] );

  /* TBD - Set exclusion bit for the given line */
  if( 1 /* TBD */ ) {
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free used memory */
  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_toggle_exclude( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of current functional unit */
  int   funit_type;       /* Type of current functional unit */
  char* sig_name;         /* Name of signal being excluded/included */
  int   value;            /* Value to set the exclusion value to */

  /* Get argument values */
  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  sig_name   = strdup_safe( argv[3], __FILE__, __LINE__ );
  value      = atoi( argv[4] );

  /* TBD - Set exclusion bit for the given toggle */
  if( 1 /* TBD */ ) {
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free used memory */
  free_safe( funit_name );
  free_safe( sig_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_comb_exclude( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of current functional unit */
  int   funit_type;       /* Type of current functional unit */
  int   expr_id;          /* Expression ID of expression to exclude/include */
  int   value;            /* Value to set the exclusion value to */

  /* Get argument values */
  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  expr_id    = atoi( argv[3] );
  value      = atoi( argv[4] );

  /* TBD - Set exclusion bit for the given expression */
  if( 1 /* TBD */ ) {
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free used memory */
  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_fsm_exclude( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of current functional unit */
  int   funit_type;       /* Type of current functional unit */
  int   line;             /* Line number that is being set */
  int   value;            /* Value to set the exclusion value to */

  /* Get argument values */
  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  line       = atoi( argv[3] );
  value      = atoi( argv[4] );

  /* TBD - Set exclusion bit for the given line */
  if( 1 /* TBD */ ) {
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free used memory */
  free_safe( funit_name );

  return( retval );

}

/*!
 \param d     TBD
 \param tcl   Pointer to the Tcl interpreter
 \param argc  Number of arguments in the argv list
 \param argv  Array of arguments passed to this function

 \return Returns TCL_OK if there are no errors encountered when running this command; otherwise, returns
         TCL_ERROR.

 Sets the exclusion value for a specified line.  This function is called whenever the user changes
 the exclusion for a specified line.  The tcl_func_get_line_summary function should be called
 immediately after to get the new line summary information.
*/
int tcl_func_set_assert_exclude( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function */
  char* funit_name;       /* Name of current functional unit */
  int   funit_type;       /* Type of current functional unit */
  int   line;             /* Line number that is being set */
  int   value;            /* Value to set the exclusion value to */

  /* Get argument values */
  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  line       = atoi( argv[3] );
  value      = atoi( argv[4] );

  /* TBD - Set exclusion bit for the given expression */
  if( 1 /* TBD */ ) {
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s", funit_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free used memory */
  free_safe( funit_name );

  return( retval );

}

/*!
 \param tcl        Pointer to Tcl interpreter structure
 \param user_home  Name of user's home directory (used to store configuration file information to)
 \param home       Name of Tcl script home directory (from running the configure script)
 \param version    Current version of Covered being run
 \param browser    Name of browser executable to use for displaying help information

 Initializes the Tcl interpreter with all procs that are created in this file.  Also sets some global
 variables that come from the environment, the configuration execution or the Covered define file.
*/
void tcl_func_initialize( Tcl_Interp* tcl, char* user_home, char* home, char* version, char* browser ) {

  Tcl_CreateCommand( tcl, "tcl_func_get_race_reason_msgs",      (Tcl_CmdProc*)(tcl_func_get_race_reason_msgs),      0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_funit_list",            (Tcl_CmdProc*)(tcl_func_get_funit_list),            0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_instance_list",         (Tcl_CmdProc*)(tcl_func_get_instance_list),         0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_filename",              (Tcl_CmdProc*)(tcl_func_get_filename),              0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_lines",   (Tcl_CmdProc*)(tcl_func_collect_uncovered_lines),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_lines",     (Tcl_CmdProc*)(tcl_func_collect_covered_lines),     0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_race_lines",        (Tcl_CmdProc*)(tcl_func_collect_race_lines),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_toggles", (Tcl_CmdProc*)(tcl_func_collect_uncovered_toggles), 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_toggles",   (Tcl_CmdProc*)(tcl_func_collect_covered_toggles),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_combs",             (Tcl_CmdProc*)(tcl_func_collect_combs),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_fsms",              (Tcl_CmdProc*)(tcl_func_collect_fsms),              0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_assertions",        (Tcl_CmdProc*)(tcl_func_collect_assertions),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_funit_start_and_end",   (Tcl_CmdProc*)(tcl_func_get_funit_start_and_end),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_toggle_coverage",       (Tcl_CmdProc*)(tcl_func_get_toggle_coverage),       0, 0 ); 
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_expression",       (Tcl_CmdProc*)(tcl_func_get_comb_expression),       0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_coverage",         (Tcl_CmdProc*)(tcl_func_get_comb_coverage),         0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_fsm_coverage",          (Tcl_CmdProc*)(tcl_func_get_fsm_coverage),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_assert_coverage",       (Tcl_CmdProc*)(tcl_func_get_assert_coverage),       0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_open_cdd",                  (Tcl_CmdProc*)(tcl_func_open_cdd),                  0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_close_cdd",                 (Tcl_CmdProc*)(tcl_func_close_cdd),                 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_save_cdd",                  (Tcl_CmdProc*)(tcl_func_save_cdd),                  0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_replace_cdd",               (Tcl_CmdProc*)(tcl_func_replace_cdd),               0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_merge_cdd",                 (Tcl_CmdProc*)(tcl_func_merge_cdd),                 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_line_summary",          (Tcl_CmdProc*)(tcl_func_get_line_summary),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_toggle_summary",        (Tcl_CmdProc*)(tcl_func_get_toggle_summary),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_summary",          (Tcl_CmdProc*)(tcl_func_get_comb_summary),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_fsm_summary",           (Tcl_CmdProc*)(tcl_func_get_fsm_summary),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_assert_summary",        (Tcl_CmdProc*)(tcl_func_get_assert_summary),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_preprocess_verilog",        (Tcl_CmdProc*)(tcl_func_preprocess_verilog),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_score_path",            (Tcl_CmdProc*)(tcl_func_get_score_path),            0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_include_pathname",      (Tcl_CmdProc*)(tcl_func_get_include_pathname),      0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_line_exclude",          (Tcl_CmdProc*)(tcl_func_set_line_exclude),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_toggle_exclude",        (Tcl_CmdProc*)(tcl_func_set_toggle_exclude),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_comb_exclude",          (Tcl_CmdProc*)(tcl_func_set_comb_exclude),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_fsm_exclude",           (Tcl_CmdProc*)(tcl_func_set_fsm_exclude),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_set_assert_exclude",        (Tcl_CmdProc*)(tcl_func_set_assert_exclude),        0, 0 );
  
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
