/*!
 \file     search.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
*/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "defines.h"
#include "search.h"
#include "link.h"
#include "module.h"


str_link* inc_paths_head = NULL;    /*!< Pointer to head element of include paths list          */
str_link* inc_paths_tail = NULL;    /*!< Pointer to tail element of include paths list          */

str_link* dir_paths_head = NULL;    /*!< Pointer to head element of directory paths list        */
str_link* dir_paths_tail = NULL;    /*!< Pointer to tail element of directory paths list        */

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

/*!
 Creates root module for module_node tree.  If a module_node points to this node as its parent,
 that node is considered the root node of the tree.
*/
void search_init() {

  module* mod;       /* Pointer to newly created module node from top module */

  mod        = module_create();
  mod->name  = strdup( top_module );
  mod->scope = strdup( top_module );

  /* Initialize module linked list */
  mod_link_add( mod, &mod_head, &mod_tail );

  /* Initialize instance tree */
  instance_add( &instance_root, NULL, mod, mod->name );

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
    tmp = strdup( path );
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

  bool  retval = TRUE;   /* Return value for this function */
  char* tmp;             /* Temporary directory name       */

  if( is_directory( path ) && directory_exists( path ) ) {
    tmp = strdup( path );
    str_link_add( tmp, &dir_paths_head, &dir_paths_tail );
  } else {
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param file Name of Verilog file to add to scoring list.
 \return Returns TRUE if the specified file exists; otherwise, returns FALSE.

*/
bool search_add_file( char* file ) {

  bool  retval = TRUE;   /* Return value for this function */
  char* tmp;             /* Temporary filename             */
  char  msg[4096];       /* Display message string         */

  if( file_exists( file ) ) {
    tmp = strdup( file );
    str_link_add( tmp, &use_files_head, &use_files_tail );
  } else {
    snprintf( msg, 4096, "File %s does not exist", file );
    print_output( msg, FATAL );
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
    tmp = strdup( module );
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

  bool  retval = TRUE;   /* Return value for this function */
  char  ext[10];         /* holder for extension           */
  char* tmp;             /* Temporary extension name       */

  if( ext_list != NULL ) {

    while( sscanf( ext_list, ".%s+", ext ) == 1 ) {
      tmp = strdup( ext );
      str_link_add( tmp, &extensions_head, &extensions_tail );
      ext_list = ext_list + strlen( ext ) + 2;
    }

    /* If extension list is not empty, we have hit a parsing error */
    if( strlen( ext_list ) > 0 ) {
      retval = FALSE;
    }

  } else {

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
  str_link_delete_list( dir_paths_head  );
  str_link_delete_list( use_files_head  );
  str_link_delete_list( no_score_head   );
  str_link_delete_list( extensions_head );

}

