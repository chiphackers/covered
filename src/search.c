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
 \file     search.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <sys/types.h>
#include <dirent.h>

#include "defines.h"
#include "search.h"
#include "link.h"
#include "func_unit.h"
#include "util.h"
#include "instance.h"


str_link* inc_paths_head  = NULL;   /*!< Pointer to head element of include paths list */
str_link* inc_paths_tail  = NULL;   /*!< Pointer to tail element of include paths list */

str_link* use_files_head  = NULL;   /*!< Pointer to head element of used files list */
str_link* use_files_tail  = NULL;   /*!< Pointer to tail element of used files list */

str_link* no_score_head   = NULL;   /*!< Pointer to head element of functional units not-to-score list */
str_link* no_score_tail   = NULL;   /*!< Pointer to tail element of functional units not-to-score list */

str_link* extensions_head = NULL;   /*!< Pointer to head element of extensions list */
str_link* extensions_tail = NULL;   /*!< Pointer to tail element of extensions list */

funit_link* funit_head    = NULL;   /*!< Pointer to head element of functional unit list */
funit_link* funit_tail    = NULL;   /*!< Pointer to tail element of functional unit list */

inst_link*  inst_head     = NULL;   /*!< Pointer to head element of functional unit instance list */
inst_link*  inst_tail     = NULL;   /*!< Pointer to tail element of functional unit instance list */

extern char*      top_module;
extern char*      top_instance;
extern char       user_msg[USER_MSG_LENGTH];
extern char**     leading_hierarchies;
extern int        leading_hier_num;
extern bool       leading_hiers_differ;
extern func_unit* global_funit;
extern func_unit* curr_funit;
extern int        flag_global_generation;

/*!
 Creates root module for module_node tree.  If a module_node points to this node as its parent,
 that node is considered the root node of the tree.
*/
void search_init() {

  func_unit* mod;            /* Pointer to newly created module node from top module */
  char       dutname[4096];  /* Instance name of top-level DUT module */
  char       lhier[4096];    /* Temporary storage of leading hierarchy */

  /* If the global generation type is SystemVerilog support, create the global $root module space */
  if( flag_global_generation == GENERATION_SV ) {

    /* Create the global functional unit */
    global_funit           = funit_create();
    global_funit->name     = strdup_safe( "$root", __FILE__, __LINE__ );
    global_funit->type     = FUNIT_MODULE;
    global_funit->filename = strdup_safe( "NA", __FILE__, __LINE__ );
    global_funit->ts_unit  = 0;
    funit_link_add( global_funit, &funit_head, &funit_tail );
    curr_funit = global_funit;

    /* Add it in the first instance tree */
    inst_link_add( instance_create( global_funit, "$root", NULL ), &inst_head, &inst_tail );
  }

  /* Now create top-level module of design */
  mod       = funit_create();
  mod->type = FUNIT_MODULE;

  if( top_module != NULL ) {
    mod->name = strdup_safe( top_module, __FILE__, __LINE__ );
  } else {
    print_output( "No top_module was specified with the -t option.  Please see \"covered -h\" for usage.",
                  FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

  /* Initialize functional unit linked list */
  funit_link_add( mod, &funit_head, &funit_tail );

  /* Initialize instance tree */
  if( top_instance == NULL ) {
    top_instance = strdup_safe( top_module, __FILE__, __LINE__ );
    inst_link_add( instance_create( mod, top_instance, NULL ), &inst_head, &inst_tail );
    leading_hierarchies = (char**)realloc( leading_hierarchies, (sizeof( char* ) * (leading_hier_num + 1)) );
    leading_hierarchies[leading_hier_num] = strdup_safe( "*", __FILE__, __LINE__ );
    leading_hier_num++;
  } else {
    scope_extract_back( top_instance, dutname, lhier );
    inst_link_add( instance_create( mod, dutname, NULL ), &inst_head, &inst_tail );
    if( lhier[0] == '\0' ) {
      leading_hierarchies = (char**)realloc( leading_hierarchies, (sizeof( char* ) * (leading_hier_num + 1)) );
      leading_hierarchies[leading_hier_num] = strdup_safe( "*", __FILE__, __LINE__ );
      leading_hier_num++;
    } else {
      leading_hierarchies = (char**)realloc( leading_hierarchies, (sizeof( char* ) * (leading_hier_num + 1)) );
      leading_hierarchies[leading_hier_num] = strdup_safe( lhier, __FILE__, __LINE__ );
      leading_hier_num++;
    }
  }

}
 
/*!
 \param path Name of include path to search for `include directives.
 \return Returns TRUE if the specified path is a legal directory string
         and specifies an existing directory; otherwise, returns FALSE.

*/
bool search_add_include_path( char* path ) {

  bool  retval = TRUE;   /* Return value for this function */
  char* tmp;             /* Temporary directory name */

  if( directory_exists( path ) ) {
    tmp = strdup_safe( path, __FILE__, __LINE__ );
    str_link_add( tmp, &inc_paths_head, &inc_paths_tail );
  } else {
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param path Name of directory to find unspecified Verilog files
 \return Returns TRUE if the specified string is a legal directory string and
         specifies an existing directory; otherwise, returns FALSE.

*/
bool search_add_directory_path( char* path ) {

  bool retval = TRUE;  /* Return value for this function */

  if( directory_exists( path ) ) {
    /* If no library extensions have been specified, assume *.v */
    if( extensions_head == NULL ) {
      str_link_add( strdup_safe( "v", __FILE__, __LINE__ ), &(extensions_head), &(extensions_tail) );
    }
    directory_load( path, extensions_head, &(use_files_head), &(use_files_tail) );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Library directory %s does not exist", path );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param file Name of Verilog file to add to scoring list.
 \return Returns TRUE if the specified file exists; otherwise, returns FALSE.

*/
bool search_add_file( char* file ) {

  bool  retval = TRUE;  /* Return value for this function */
  char* tmp;            /* Temporary filename */

  if( file_exists( file ) ) {
    if( str_link_find( file, use_files_head ) == NULL ) {
      tmp = strdup_safe( file, __FILE__, __LINE__ );
      str_link_add( tmp, &use_files_head, &use_files_tail );
    }
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "File %s does not exist", file );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param funit  Name of functional unit to specifically not score

 \return Returns TRUE if the specified string is a legal functional unit name; otherwise,
         returns FALSE.

*/
bool search_add_no_score_funit( char* funit ) {

  bool  retval = TRUE;   /* Return value for this function */
  char* tmp;             /* Temporary module name */

  if( is_func_unit( funit ) ) {
    tmp = strdup_safe( funit, __FILE__, __LINE__ );
    str_link_add( tmp, &no_score_head, &no_score_tail );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Value of -e option (%s) is not a valid block name", funit );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param ext_list String containing extensions to allow in search.

 \return Returns TRUE if the specified string is in the correct format;
         otherwise, returns FALSE.

 Parses the given +libext argument, extracting all extensions listed and storing them into
 the globally accessible extensions list.
*/
bool search_add_extensions( char* ext_list ) {

  bool  retval    = TRUE;      /* Return value for this function */
  char  ext[30];               /* Holder for extension */
  int   ext_index = 0;         /* Index to ext array */
  char* tmp       = ext_list;  /* Temporary extension name */

  assert( ext_list != NULL );

  while( (*tmp != '\0') && retval ) {
    assert( ext_index < 30 );
    if( *tmp == '+' ) { 
      ext[ext_index] = '\0';
      ext_index      = 0;
      str_link_add( strdup_safe( ext, __FILE__, __LINE__ ), &extensions_head, &extensions_tail );
    } else if( *tmp == '.' ) {
      if( ext_index > 0 ) {
        retval = FALSE;
      }
    } else {
      ext[ext_index] = *tmp;
      ext_index++;
    }
    tmp++;
  }

  /* If extension list is not empty, we have hit a parsing error */
  if( (strlen( tmp ) > 0) || (ext_index > 0) ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Parsing error in +libext+%s  ", ext_list );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    gen_space( user_msg, (25 + (strlen( ext_list ) - strlen( tmp ))) );
    strcat( user_msg, "^" );
    print_output( user_msg, FATAL_WRAP, __FILE__, __LINE__ );
    retval = FALSE;
  }

  return( retval );

}

/*!
 This function should be called after all parsing is completed.  
 Deletes all initialized lists.
*/
void search_free_lists() {

  str_link_delete_list( inc_paths_head  );
  str_link_delete_list( use_files_head  );
  str_link_delete_list( no_score_head   );
  str_link_delete_list( extensions_head );

}

/*
 $Log$
 Revision 1.29.2.1  2007/03/13 22:05:11  phase1geo
 Fixing bug 1678931.  Updated regression.

 Revision 1.29  2006/11/25 21:29:01  phase1geo
 Adding timescale diagnostics to regression suite and fixing bugs in core associated
 with this code.  Full regression now passes for IV and Cver (not in VPI mode).

 Revision 1.28  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.27  2006/08/31 22:32:18  phase1geo
 Things are in a state of flux at the moment.  I have added proper parsing support
 for assertions, properties and sequences.  Also added partial support for the $root
 space (though I need to work on figuring out how to handle it in terms of the
 instance tree) and all that goes along with that.  Add parsing support with an
 error message for multi-dimensional array declarations.  Regressions should not be
 expected to run correctly at the moment.

 Revision 1.26  2006/07/12 22:16:18  phase1geo
 Fixing hierarchical referencing for instance arrays.  Also attempted to fix
 a problem found with unary1; however, the generated report coverage information
 does not look correct at this time.  Checkpointing what I have done for now.

 Revision 1.25  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.24  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.23  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.22  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.21  2006/01/14 04:17:23  phase1geo
 Adding is_func_unit function to check to see if a -e value is a valid module, function,
 task or named begin/end block.  Updated regression accordingly.  We are getting closer
 but still have a few diagnostics to figure out yet.

 Revision 1.20  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.19  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.18  2005/01/03 23:00:35  phase1geo
 Fixing library extension parser.

 Revision 1.17  2005/01/03 22:02:27  phase1geo
 Fixing case where file is specified with -v after it has already been included to
 just ignore the file instead of printing out an incorrect error message that the
 file was unable to be opened.

 Revision 1.16  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.15  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.14  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.13  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.12  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.11  2002/10/13 13:55:53  phase1geo
 Fixing instance depth selection and updating all configuration files for
 regression.  Full regression now passes.

 Revision 1.10  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.9  2002/08/19 21:36:26  phase1geo
 Fixing memory corruption bug in score function for adding Verilog modules
 to use_files list.  This caused a core dump to occur when the -f option
 was used.

 Revision 1.8  2002/07/20 20:48:09  phase1geo
 Fixing a bug that caused the same file to be added to the use_files list
 more than once.  A filename will only appear once in this list now.  Updates
 to the TODO list.

 Revision 1.7  2002/07/18 05:50:45  phase1geo
 Fixes should be just about complete for instance depth problems now.  Diagnostics
 to help verify instance handling are added to regression.  Full regression passes.

 Revision 1.6  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.5  2002/07/11 19:12:38  phase1geo
 Fixing version number.  Fixing bug with score command if -t option was not
 specified to avoid a segmentation fault.

 Revision 1.4  2002/07/08 19:02:12  phase1geo
 Adding -i option to properly handle modules specified for coverage that
 are instantiated within a design without needing to parse parent modules.

 Revision 1.3  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

