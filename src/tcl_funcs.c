/*!
 \file     tcl_funcs.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/26/2004
*/

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


extern mod_link* mod_head;
extern mod_inst* instance_root;
extern char      user_msg[USER_MSG_LENGTH];


int tcl_func_get_module_list( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] ) {

  char** mod_list;         /* List of module names in design */
  int    mod_size;         /* Number of elements in mod_list */
  int    retval = TCL_OK;  /* Return value of this function  */
  int    i;                /* Loop iterator                  */

  if( module_get_list( &mod_list, &mod_size ) ) {
    for( i=0; i<mod_size; i++ ) {
      Tcl_SetVar( tcl, "mod_list", mod_list[i], (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }
  } else {
    retval = TCL_ERROR;
  }

  return( retval );

}

int tcl_func_get_instances( Tcl_Interp* tcl, mod_inst* root ) {

  mod_inst* curr;
  char      scope[4096];  /* Hierarchical scope name */

  /* Generate the name of this child */
  scope[0] = '\0';
  instance_gen_scope( scope, root );
  Tcl_SetVar( tcl, "inst_list", scope,           (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
  Tcl_SetVar( tcl, "mod_list",  root->mod->name, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );

  curr = root->child_head;
  while( curr != NULL ) {
    tcl_func_get_instances( tcl, curr );
    curr = curr->next;
  }

  return( 0 );

}

int tcl_func_get_instance_list( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  mod_inst* inst;             /* Pointer to current instance    */
  int       retval = TCL_OK;  /* Return value for this function */

  if( instance_root != NULL ) {
    tcl_func_get_instances( tcl, instance_root );
  }

  return( retval );

}

int tcl_func_get_filename( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  char* filename;
  int   retval = TCL_OK;

  if( (filename = module_get_filename( argv[1] )) != NULL ) {
    Tcl_SetVar( tcl, "file_name", filename, TCL_GLOBAL_ONLY );
  } else {
    retval = TCL_ERROR;
  }

  return( retval );

}

int tcl_func_get_module_start_and_end( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int  retval = TCL_OK;  /* Return value for this function */
  int  start_line;
  int  end_line;
  char linenum[20];

  if( module_get_start_and_end_lines( argv[1], &start_line, &end_line ) ) {
    snprintf( linenum, 20, "%d", start_line );
    Tcl_SetVar( tcl, "start_line", linenum, TCL_GLOBAL_ONLY );
    snprintf( linenum, 20, "%d", end_line );
    Tcl_SetVar( tcl, "end_line",   linenum, TCL_GLOBAL_ONLY );
  } else {
    retval = TCL_ERROR;
  }

  return( retval );

}

int tcl_func_collect_uncovered_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval  = TCL_OK;
  int*  lines;
  int   line_cnt;
  int   i;
  char  line[20];

  if( line_collect( argv[1], 0, &lines, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line, 20, "%d", lines[i] );
      Tcl_SetVar( tcl, "uncovered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }

    free_safe( lines );

  } else {

    retval = TCL_ERROR;

  }

  return( retval );

}

int tcl_func_collect_covered_lines( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval  = TCL_OK;
  char* modname;
  int*  lines;
  int   line_cnt;
  int   i;
  char  line[20];

  modname = strdup_safe( argv[1], __FILE__, __LINE__ );

  if( line_collect( modname, 1, &lines, &line_cnt ) ) {

    for( i=0; i<line_cnt; i++ ) {
      snprintf( line, 20, "%d", lines[i] );
      Tcl_SetVar( tcl, "covered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
    }

    free_safe( lines );

  } else {

    retval = TCL_ERROR;

  }

  free_safe( modname );

  return( retval );

}

int tcl_func_open_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* ifile;

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1], __FILE__, __LINE__ );

    if( !report_read_cdd_and_ready( ifile, READ_MODE_REPORT_MOD_MERGE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open CDD \"%s\"", ifile );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    }

    free_safe( ifile );

  }

  return( retval );

}

int tcl_func_replace_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* ifile;

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1], __FILE__, __LINE__ );

    if( !report_read_cdd_and_ready( ifile, READ_MODE_REPORT_MOD_REPLACE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to replace current CDD with \"%s\"", ifile );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    }

    free_safe( ifile );

  }

  return( retval );

}

int tcl_func_merge_cdd( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* ifile;

  /* If no filename was specified, the user hit cancel so just exit gracefully */
  if( argv[1][0] != '\0' ) {

    ifile = strdup_safe( argv[1], __FILE__, __LINE__ );

    if( !report_read_cdd_and_ready( ifile, READ_MODE_REPORT_MOD_MERGE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to merge current CDD with \"%s\"", ifile );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = TCL_ERROR;
    }

    free_safe( ifile );

  }

  return( retval );

}

int tcl_func_get_line_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function           */
  char* mod_name;         /* Name of module to lookup                 */
  int   total;            /* Contains total number of lines evaluated */
  int   hit;              /* Contains total number of lines hit       */
  char  value[20];        /* String version of a value                */

  mod_name = strdup_safe( argv[1], __FILE__, __LINE__ );
 
  if( line_get_module_summary( mod_name, &total, &hit ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "line_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "line_summary_hit", value, TCL_GLOBAL_ONLY );
  } else {
    retval = TCL_ERROR;
  }

  free_safe( mod_name );

  return( retval );

}

void tcl_func_initialize( Tcl_Interp* tcl, char* home ) {

  Tcl_CreateCommand( tcl, "tcl_func_get_module_list",          (Tcl_CmdProc*)(tcl_func_get_module_list),          0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_instance_list",        (Tcl_CmdProc*)(tcl_func_get_instance_list),        0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_filename",             (Tcl_CmdProc*)(tcl_func_get_filename),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_lines",  (Tcl_CmdProc*)(tcl_func_collect_uncovered_lines),  0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_lines",    (Tcl_CmdProc*)(tcl_func_collect_covered_lines),    0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_module_start_and_end", (Tcl_CmdProc*)(tcl_func_get_module_start_and_end), 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_open_cdd",                 (Tcl_CmdProc*)(tcl_func_open_cdd),                 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_replace_cdd",              (Tcl_CmdProc*)(tcl_func_replace_cdd),              0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_merge_cdd",                (Tcl_CmdProc*)(tcl_func_merge_cdd),                0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_line_summary",         (Tcl_CmdProc*)(tcl_func_get_line_summary),         0, 0 );

  /* Set HOME variable to location of scripts */
  Tcl_SetVar( tcl, "HOME", home, TCL_GLOBAL_ONLY );

}

/*
 $Log$
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
