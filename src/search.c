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
#include "module.h"
#include "util.h"
#include "instance.h"


str_link* inc_paths_head = NULL;    /*!< Pointer to head element of include paths list          */
str_link* inc_paths_tail = NULL;    /*!< Pointer to tail element of include paths list          */

str_link* use_files_head = NULL;    /*!< Pointer to head element of used files list             */
str_link* use_files_tail = NULL;    /*!< Pointer to tail element of used files list             */

str_link* no_score_head = NULL;     /*!< Pointer to head element of modules not-to-score list   */
str_link* no_score_tail = NULL;     /*!< Pointer to tail element of modules not-to-score list   */

str_link* extensions_head = NULL;   /*!< Pointer to head element of extensions list             */
str_link* extensions_tail = NULL;   /*!< Pointer to tail element of extensions list             */

mod_link* mod_head       = NULL;    /*!< Pointer to head element of module list */
mod_link* mod_tail       = NULL;    /*!< Pointer to tail element of module list */

mod_inst* instance_root = NULL;     /*!< Pointer to root of module instance tree */

extern char* top_module;
extern char* top_instance;
extern char  user_msg[USER_MSG_LENGTH];
extern char  leading_hierarchy[4096];

/*!
 Creates root module for module_node tree.  If a module_node points to this node as its parent,
 that node is considered the root node of the tree.
*/
void search_init() {

  module* mod;            /* Pointer to newly created module node from top module */
  char    dutname[4096];  /* Instance name of top-level DUT module                */

  mod = module_create();

  if( top_module != NULL ) {
    mod->name  = strdup_safe( top_module, __FILE__, __LINE__ );
  } else {
    print_output( "No top_module was specified with the -t option.  Please see \"covered -h\" for usage.",
                  FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

  /* Initialize module linked list */
  mod_link_add( mod, &mod_head, &mod_tail );

  /* Initialize instance tree */
  if( top_instance == NULL ) {
    top_instance = strdup_safe( top_module, __FILE__, __LINE__ );
    instance_parse_add( &instance_root, NULL, mod, strdup_safe( top_instance, __FILE__, __LINE__ ) );
    leading_hierarchy[0] = '*';
    leading_hierarchy[1] = '\0';
  } else {
    scope_extract_back( top_instance, dutname, leading_hierarchy );
    instance_parse_add( &instance_root, NULL, mod, strdup_safe( dutname, __FILE__, __LINE__ ) );
    if( leading_hierarchy[0] == '\0' ) {
      leading_hierarchy[0] = '*';
      leading_hierarchy[1] = '\0';
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
  char* tmp;             /* Temporary directory name       */

  if( is_directory( path ) ) {
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

  if( is_directory( path ) && directory_exists( path ) ) {
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
  char* tmp;            /* Temporary filename             */

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
 \param module Name of module to specifically not score
 \return Returns TRUE if the specified string is a legal module name; otherwise,
         returns FALSE.

*/
bool search_add_no_score_module( char* module ) {

  bool  retval = TRUE;   /* Return value for this function */
  char* tmp;             /* Temporary module name          */

  if( is_variable( module ) ) {
    tmp = strdup_safe( module, __FILE__, __LINE__ );
    str_link_add( tmp, &no_score_head, &no_score_tail );
  } else {
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param ext_list String containing extensions to allow in search.
 \return Returns TRUE if the specified string is in the correct format;
         otherwise, returns FALSE.

*/
bool search_add_extensions( char* ext_list ) {

  bool  retval    = TRUE;      /* Return value for this function */
  char  ext[30];               /* holder for extension           */
  int   ext_index = 0;         /* Index to ext array             */
  char* tmp       = ext_list;  /* Temporary extension name       */

  if( ext_list != NULL ) {

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
    if( strlen( tmp ) > 0 ) {
      retval = FALSE;
    }

  } else {

    retval = FALSE;

  }

  /* If we had a parsing error, output the problem here */
  if( !retval ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Bad library extension specified (+libext+%s)\n", ext_list );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
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

