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
#include "line.h"
#include "toggle.h"
#include "comb.h"
#include "expr.h"


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
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to get modules list from this design" );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
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

  return( TCL_OK );

}

int tcl_func_get_instance_list( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  mod_inst* inst;             /* Pointer to current instance    */
  int       retval = TCL_OK;  /* Return value for this function */

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

  if( (filename = module_get_filename( argv[1] )) != NULL ) {
    Tcl_SetVar( tcl, "file_name", filename, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find filename for module %s", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
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
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find start and end lines for module %s", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
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
      if( i == 0 ) {
        Tcl_SetVar( tcl, "uncovered_lines", line, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "uncovered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
    }

    free_safe( lines );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
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
      if( i == 0 ) {
        Tcl_SetVar( tcl, "covered_lines", line, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "covered_lines", line, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
    }

    free_safe( lines );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( modname );

  return( retval );

}

int tcl_func_collect_uncovered_toggles( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int          retval = TCL_OK;
  char*        modname;
  expression** sigs;
  int          sig_cnt;
  int          i;
  char         str[85];
  int          startline;

  modname   = strdup_safe( argv[1], __FILE__, __LINE__ );
  startline = atol( argv[2] );

  if( toggle_collect( modname, 0, &sigs, &sig_cnt ) ) {

    for( i=0; i<sig_cnt; i++ ) {
      snprintf( str, 85, "%d.%d %d.%d", (sigs[i]->line - (startline - 1)), (((sigs[i]->col >> 16) & 0xffff) + 9),
                                        (sigs[i]->line - (startline - 1)), ((sigs[i]->col & 0xffff) + 10) );
      if( i == 0 ) { 
        Tcl_SetVar( tcl, "uncovered_toggles", str, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "uncovered_toggles", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
    }

    free_safe( sigs );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( modname );

  return( retval );

}

int tcl_func_collect_covered_toggles( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int          retval = TCL_OK;
  char*        modname;
  expression** sigs;
  int          sig_cnt;
  int          i;
  char         str[85];
  int          startline;

  modname   = strdup_safe( argv[1], __FILE__, __LINE__ );
  startline = atol( argv[2] );

  if( toggle_collect( modname, 1, &sigs, &sig_cnt ) ) {

    for( i=0; i<sig_cnt; i++ ) {
      snprintf( str, 85, "%d.%d %d.%d", (sigs[i]->line - (startline - 1)), (((sigs[i]->col >> 16) & 0xffff) + 9),
                                        (sigs[i]->line - (startline - 1)), ((sigs[i]->col & 0xffff) + 10) );
      if( i == 0 ) {
        Tcl_SetVar( tcl, "covered_toggles", str, (TCL_GLOBAL_ONLY | TCL_LIST_ELEMENT) );
      } else {
        Tcl_SetVar( tcl, "covered_toggles", str, (TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) );
      }
    }

    free_safe( sigs );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( modname );

  return( retval );

}

int tcl_func_get_toggle_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;
  char* modname;
  char* signame;
  int   msb;
  int   lsb; 
  char* tog01;
  char* tog10;
  char  tmp[20];

  modname = strdup_safe( argv[1], __FILE__, __LINE__ );
  signame = strdup_safe( argv[2], __FILE__, __LINE__ );

  if( toggle_get_coverage( modname, signame, &msb, &lsb, &tog01, &tog10 ) ) {

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
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free up allocated memory */
  free_safe( modname );
  free_safe( signame );

  return( retval );

}

int tcl_func_collect_combs( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int          retval = TCL_OK;
  char*        modname;
  expression** covs;
  expression** uncovs;
  int          cov_cnt;
  int          uncov_cnt;
  int          i;
  char         str[85];
  int          startline;
  expression*  last;

  modname   = strdup_safe( argv[1], __FILE__, __LINE__ );
  startline = atol( argv[2] );

  if( combination_collect( modname, &covs, &cov_cnt, &uncovs, &uncov_cnt ) ) {

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

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;

  }

  free_safe( modname );

  return( retval );

}

int tcl_func_get_comb_expression( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int    retval = TCL_OK;
  char*  modname;
  int    expr_id;
  char** code;
  int*   uline_groups;
  int    code_size;
  char** ulines;
  int    uline_size;
  int    i;
  char   tmp[20];

  modname = strdup_safe( argv[1], __FILE__, __LINE__ );
  expr_id = atol( argv[2] );

  if( combination_get_expression( modname, expr_id, &code, &uline_groups, &code_size, &ulines, &uline_size ) ) {

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
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s in design", argv[1] );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  /* Free up allocated memory */
  free_safe( modname );

  return( retval );

}

int tcl_func_get_comb_coverage( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int    retval = TCL_OK;  /* Return value for this function                      */
  char*  modname;          /* Name of module containing expression to lookup      */
  int    ulid;             /* Underline ID of expression to find                  */
  char** info;             /* Array containing lines of coverage information text */
  int    info_size;        /* Specifies number of elements in info array          */
  int    i;                /* Loop iterator                                       */

  modname = strdup_safe( argv[1], __FILE__, __LINE__ );
  ulid    = atol( argv[2] );

  if( combination_get_coverage( modname, ulid, &info, &info_size ) ) {

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

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s and/or expression ID %d in design", argv[1], ulid );
    Tcl_AddErrorInfo( tcl, user_msg );
    retval = TCL_ERROR;
  }

  /* Free up allocated memory */
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
      Tcl_AddErrorInfo( tcl, user_msg );
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
      Tcl_AddErrorInfo( tcl, user_msg );
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
      Tcl_AddErrorInfo( tcl, user_msg );
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
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s", mod_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( mod_name );

  return( retval );

}

int tcl_func_get_toggle_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function             */
  char* mod_name;         /* Name of module to lookup                   */
  int   total;            /* Contains total number of toggles evaluated */
  int   hit01;            /* Contains total number of toggle 0->1 hit   */
  int   hit10;            /* Contains total number of toggle 1->0 hit   */
  char  value[20];        /* String version of a value                  */

  mod_name = strdup_safe( argv[1], __FILE__, __LINE__ );
		     
  if( toggle_get_module_summary( mod_name, &total, &hit01, &hit10 ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "toggle_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit01 );
    Tcl_SetVar( tcl, "toggle_summary_hit01", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit10 );
    Tcl_SetVar( tcl, "toggle_summary_hit10", value, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s", mod_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( mod_name );

  return( retval );

}

int tcl_func_get_comb_summary( ClientData d, Tcl_Interp* tcl, int argc, const char* argv[] ) {

  int   retval = TCL_OK;  /* Return value for this function                 */
  char* mod_name;         /* Name of module to lookup                       */
  int   total;            /* Contains total number of expressions evaluated */
  int   hit;              /* Contains total number of expressions hit       */
  char  value[20];        /* String version of a value                      */

  mod_name = strdup_safe( argv[1], __FILE__, __LINE__ );

  if( combination_get_module_summary( mod_name, &total, &hit ) ) {
    snprintf( value, 20, "%d", total );
    Tcl_SetVar( tcl, "comb_summary_total", value, TCL_GLOBAL_ONLY );
    snprintf( value, 20, "%d", hit );
    Tcl_SetVar( tcl, "comb_summary_hit", value, TCL_GLOBAL_ONLY );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find module %s", mod_name );
    Tcl_AddErrorInfo( tcl, user_msg );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = TCL_ERROR;
  }

  free_safe( mod_name );

  return( retval );

}

void tcl_func_initialize( Tcl_Interp* tcl, char* home, char* browser ) {

  Tcl_CreateCommand( tcl, "tcl_func_get_module_list",           (Tcl_CmdProc*)(tcl_func_get_module_list),           0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_instance_list",         (Tcl_CmdProc*)(tcl_func_get_instance_list),         0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_filename",              (Tcl_CmdProc*)(tcl_func_get_filename),              0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_lines",   (Tcl_CmdProc*)(tcl_func_collect_uncovered_lines),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_lines",     (Tcl_CmdProc*)(tcl_func_collect_covered_lines),     0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_uncovered_toggles", (Tcl_CmdProc*)(tcl_func_collect_uncovered_toggles), 0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_covered_toggles",   (Tcl_CmdProc*)(tcl_func_collect_covered_toggles),   0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_collect_combs",             (Tcl_CmdProc*)(tcl_func_collect_combs),             0, 0 );
  Tcl_CreateCommand( tcl, "tcl_func_get_module_start_and_end",  (Tcl_CmdProc*)(tcl_func_get_module_start_and_end),  0, 0 );
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

  /* Set BROWSER variable to locate browser to use for help pages */
  Tcl_SetVar( tcl, "BROWSER", browser, TCL_GLOBAL_ONLY );

}

/*
 $Log$
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
