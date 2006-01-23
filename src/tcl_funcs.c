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


extern funit_link* funit_head;
extern funit_inst* instance_root;
extern char        user_msg[USER_MSG_LENGTH];
extern const char* race_msgs[RACE_TYPE_NUM];


int tcl_func_get_race_reason_msgs( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] ) {

  int retval = TCL_OK;  /* Return value of this function */
  int i;                /* Loop iterator                 */

  for( i=0; i<RACE_TYPE_NUM; i++ ) {
    strcpy( user_msg, race_msgs[i] );
    if( i == 0 ) {
      Tcl_SetVar( tcl, "race_msgs", user_msg, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
    } else {
      Tcl_SetVar( tcl, "race_msgs", user_msg, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }
  }

  return( retval );

}

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

int tcl_func_get_filename( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  char* filename;
  int   retval = TCL_OK;

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

int tcl_func_get_funit_start_and_end( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int  retval = TCL_OK;  /* Return value for this function */
  int  start_line;
  int  end_line;
  char linenum[20];

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

int tcl_func_collect_uncovered_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* funit_name;
  int   funit_type;
  int*  lines;
  int   line_cnt;
  int   i;
  char  line[20];

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( line_collect( funit_name, funit_type, 0, &lines, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line, 20, "%d", lines[i] );
      if( i == 0 ) {
        Tcl_SetVar( tcl, "uncovered_lines", line, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "uncovered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
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

int tcl_func_collect_covered_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval  = TCL_OK;
  char* funit_name;
  int   funit_type;
  int*  lines;
  int   line_cnt;
  int   i;
  char  line[20];

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( line_collect( funit_name, funit_type, 1, &lines, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line, 20, "%d", lines[i] );
      if( i == 0 ) {
        Tcl_SetVar( tcl, "covered_lines", line, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "covered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
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

int tcl_func_collect_race_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval  = TCL_OK;
  char* funit_name;
  int   funit_type;
  int*  lines;
  int*  reasons;
  int   line_cnt;
  int   i       = 0;
  char  line[20]; 
  char  reason[20];

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );

  if( race_collect_lines( funit_name, funit_type, &lines, &reasons, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line,   20, "%d", lines[i]   );
      snprintf( reason, 20, "%d", reasons[i] );
      if( i == 0 ) {
	Tcl_SetVar( tcl, "race_lines",   line,   (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
        Tcl_SetVar( tcl, "race_reasons", reason, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
	Tcl_SetVar( tcl, "race_lines",   line,   (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
	Tcl_SetVar( tcl, "race_reasons", reason, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
    }

    free_safe( lines );
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
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 9),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 10) );
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
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + 9),
                (sigl->sig->line - (start_line - 1)), (sigl->sig->suppl.part.col + (strlen( sigl->sig->name ) - 1) + 10) );
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
*/
int tcl_func_get_toggle_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* funit_name;
  int   funit_type;
  char* signame;
  int   msb;
  int   lsb; 
  char* tog01;
  char* tog10;
  char  tmp[20];

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
*/
int tcl_func_collect_combs( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int          retval = TCL_OK;
  char*        funit_name;
  int          funit_type;
  expression** covs;
  expression** uncovs;
  int          cov_cnt;
  int          uncov_cnt;
  int          i;
  char         str[85];
  int          startline;
  expression*  last;

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  startline  = atoi( argv[3] );

  if( combination_collect( funit_name, funit_type, &covs, &cov_cnt, &uncovs, &uncov_cnt ) ) {

    /* Load uncovered statements into Tcl */
    for( i=0; i<uncov_cnt; i++ ) {
      last = expression_get_last_line_expr( uncovs[i] );
      snprintf( str, 85, "%d.%d %d.%d %d", (uncovs[i]->line - (startline - 1)), (((uncovs[i]->col >> 16) & 0xffff) + 9),
                                           (last->line      - (startline - 1)), ((last->col              & 0xffff) + 10),
                                           uncovs[i]->id );
      if( i == 0 ) {
        Tcl_SetVar( tcl, "uncovered_combs", str, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "uncovered_combs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
    }

    /* Load covered statements into Tcl */
    for( i=0; i<cov_cnt; i++ ) {
      last = expression_get_last_line_expr( covs[i] );
      snprintf( str, 85, "%d.%d %d.%d", (covs[i]->line - (startline - 1)), (((covs[i]->col >> 16) & 0xffff) + 9),
                                           (last->line    - (startline - 1)), ((last->col            & 0xffff) + 10) );
      if( i == 0 ) {
        Tcl_SetVar( tcl, "covered_combs", str, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "covered_combs", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
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
*/
int tcl_func_get_comb_expression( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int    retval = TCL_OK;
  char*  funit_name;
  int    funit_type;
  int    expr_id;
  char** code;
  int*   uline_groups;
  int    code_size;
  char** ulines;
  int    uline_size;
  int    i;
  char   tmp[20];

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  expr_id    = atoi( argv[3] );

  if( combination_get_expression( funit_name, funit_type, expr_id, &code, &uline_groups, &code_size, &ulines, &uline_size ) ) {

    for( i=0; i<code_size; i++ ) {
      if( i == 0 ) {
        Tcl_SetVar( tcl, "comb_code",         code[i], (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
        snprintf( tmp, 20, "%d", uline_groups[i] );
        Tcl_SetVar( tcl, "comb_uline_groups", tmp,     (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "comb_code",         code[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
        snprintf( tmp, 20, "%d", uline_groups[i] );
        Tcl_SetVar( tcl, "comb_uline_groups", tmp,     (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
      free_safe( code[i] );
    }

    for( i=0; i<uline_size; i++ ) {
      if( i == 0 ) {
        Tcl_SetVar( tcl, "comb_ulines", ulines[i], (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "comb_ulines", ulines[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
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
*/
int tcl_func_get_comb_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int    retval = TCL_OK;  /* Return value for this function                          */
  char*  funit_name;       /* Name of functional unit containing expression to lookup */
  int    funit_type;       /* Type of functional unit containing expression to lookup */
  int    ulid;             /* Underline ID of expression to find                      */
  char** info;             /* Array containing lines of coverage information text     */
  int    info_size;        /* Specifies number of elements in info array              */
  int    i;                /* Loop iterator                                           */

  funit_name = strdup_safe( argv[1], __FILE__, __LINE__ );
  funit_type = atoi( argv[2] );
  ulid       = atoi( argv[3] );

  if( combination_get_coverage( funit_name, funit_type, ulid, &info, &info_size ) ) {

    if( info_size > 0 ) {

      for( i=0; i<info_size; i++ ) {
        if( i == 0 ) {
          Tcl_SetVar( tcl, "comb_expr_cov", info[i], (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
        } else {
          Tcl_SetVar( tcl, "comb_expr_cov", info[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
        }
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
*/
int tcl_func_open_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* ifile;

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
*/
int tcl_func_replace_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* ifile;

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
*/
int tcl_func_merge_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* ifile;

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1], __FILE__, __LINE__ );

    if( !report_read_cdd_and_ready( ifile, READ_MODE_REPORT_MOD_MERGE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to merge current CDD with \"%s\"", ifile );
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
*/
int tcl_func_get_line_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function           */
  char* funit_name;       /* Name of functional unit to lookup        */
  int   funit_type;       /* Type of functional unit to lookup        */
  int   total;            /* Contains total number of lines evaluated */
  int   hit;              /* Contains total number of lines hit       */
  char  value[20];        /* String version of a value                */

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

void tcl_func_initialize( Tcl_Interp* tcl, char* home, char* version, char* browser ) {

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
  Tcl_CreateCommand( tcl, "tcl_func_get_funit_start_and_end",   (Tcl_CmdProc*)(tcl_func_get_funit_start_and_end),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_toggle_coverage",       (Tcl_CmdProc*)(tcl_func_get_toggle_coverage),       0, 0 ); 
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_expression",       (Tcl_CmdProc*)(tcl_func_get_comb_expression),       0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_coverage",         (Tcl_CmdProc*)(tcl_func_get_comb_coverage),         0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_open_cdd",                  (Tcl_CmdProc*)(tcl_func_open_cdd),                  0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_replace_cdd",               (Tcl_CmdProc*)(tcl_func_replace_cdd),               0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_merge_cdd",                 (Tcl_CmdProc*)(tcl_func_merge_cdd),                 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_line_summary",          (Tcl_CmdProc*)(tcl_func_get_line_summary),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_toggle_summary",        (Tcl_CmdProc*)(tcl_func_get_toggle_summary),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_comb_summary",          (Tcl_CmdProc*)(tcl_func_get_comb_summary),          0, 0 );

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
