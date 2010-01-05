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
 \file     search.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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


/*@null@*/        str_link* inc_paths_head  = NULL;   /*!< Pointer to head element of include paths list */
/*@null@*/ static str_link* inc_paths_tail  = NULL;   /*!< Pointer to tail element of include paths list */

/*@null@*/        str_link* use_files_head  = NULL;   /*!< Pointer to head element of used files list */
/*@null@*/ static str_link* use_files_tail  = NULL;   /*!< Pointer to tail element of used files list */

/*@null@*/        str_link* no_score_head   = NULL;   /*!< Pointer to head element of functional units not-to-score list */
/*@null@*/ static str_link* no_score_tail   = NULL;   /*!< Pointer to tail element of functional units not-to-score list */

/*@null@*/ static str_link* extensions_head = NULL;   /*!< Pointer to head element of extensions list */
/*@null@*/ static str_link* extensions_tail = NULL;   /*!< Pointer to tail element of extensions list */

extern db**         db_list;
extern unsigned int db_size;
extern unsigned int curr_db;
extern char*        top_module;
extern char*        top_instance;
extern char         user_msg[USER_MSG_LENGTH];
extern func_unit*   global_funit;
extern func_unit*   curr_funit;
extern unsigned int flag_global_generation;

/*!
 \throws anonymous Throw

 Creates root module for module_node tree.  If a module_node points to this node as its parent,
 that node is considered the root node of the tree.
*/
void search_init() { PROFILE(SEARCH_INIT);

  func_unit* mod;            /* Pointer to newly created module node from top module */
  char       dutname[4096];  /* Instance name of top-level DUT module */
  char       lhier[4096];    /* Temporary storage of leading hierarchy */

  /* Check to make sure that the user specified a -t option value */
  if( top_module == NULL ) {
    print_output( "No top module was specified with the -t option.  Please see \"covered -h\" for usage.",
                  FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* If the global generation type is SystemVerilog support, create the global $root module space */
  if( flag_global_generation == GENERATION_SV ) {

    /* Create the global functional unit */
    global_funit                  = funit_create();
    global_funit->name            = strdup_safe( "$root" );
    global_funit->suppl.part.type = FUNIT_MODULE;
    global_funit->orig_fname      = strdup_safe( "NA" );
    global_funit->ts_unit         = 2;
    (void)funit_link_add( global_funit, &(db_list[curr_db]->funit_head), &(db_list[curr_db]->funit_tail) );
    curr_funit = global_funit;

    /* Add it in the first instance tree */
    (void)inst_link_add( instance_create( global_funit, "$root", 0, 0, FALSE, FALSE, FALSE, NULL ), &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
  }

  /* Now create top-level module of design */
  mod                  = funit_create();
  mod->suppl.part.type = FUNIT_MODULE;
  mod->name = strdup_safe( top_module );

  /* Initialize functional unit linked list */
  (void)funit_link_add( mod, &(db_list[curr_db]->funit_head), &(db_list[curr_db]->funit_tail) );

  /* Initialize instance tree */
  if( top_instance == NULL ) {
    top_instance = strdup_safe( top_module );
    (void)inst_link_add( instance_create( mod, top_instance, 0, 0, FALSE, FALSE, FALSE, NULL ), &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
    db_list[curr_db]->leading_hierarchies = (char**)realloc_safe( db_list[curr_db]->leading_hierarchies, (sizeof( char* ) * db_list[curr_db]->leading_hier_num), (sizeof( char* ) * (db_list[curr_db]->leading_hier_num + 1)) );
    db_list[curr_db]->leading_hierarchies[db_list[curr_db]->leading_hier_num] = strdup_safe( "*" );
    db_list[curr_db]->leading_hier_num++;
  } else {
    scope_extract_back( top_instance, dutname, lhier );
    if( lhier[0] == '\0' ) {
      db_list[curr_db]->leading_hierarchies = (char**)realloc_safe( db_list[curr_db]->leading_hierarchies, (sizeof( char* ) * db_list[curr_db]->leading_hier_num), (sizeof( char* ) * (db_list[curr_db]->leading_hier_num + 1)) );
      db_list[curr_db]->leading_hierarchies[db_list[curr_db]->leading_hier_num] = strdup_safe( "*" );
      db_list[curr_db]->leading_hier_num++;
      (void)inst_link_add( instance_create( mod, dutname, 0, 0, FALSE, FALSE, FALSE, NULL ), &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
    } else {
      char        tmp1[4096];
      char        tmp2[4096];
      char        tmp3[4096];
      inst_link*  instl;
      funit_inst* parent;
      funit_inst* child;
      (void)strcpy( tmp1, lhier );
      scope_extract_front( tmp1, tmp2, tmp3 );
      instl  = inst_link_add( instance_create( NULL, tmp2, 0, 0, FALSE, FALSE, FALSE, NULL ), &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
      parent = instl->inst;
      while( tmp3[0] != '\0' ) {
        (void)strcpy( tmp1, tmp3 );
        scope_extract_front( tmp1, tmp2, tmp3 );
        child = instance_create( NULL, tmp2, 0, 0, FALSE, FALSE, FALSE, NULL );
        child->parent = parent;
        if( parent->child_head == NULL ) {
          parent->child_head       = child;
          parent->child_tail       = child;
        } else {
          parent->child_tail->next = child;
          parent->child_tail       = child;
        }
        parent = child;
      }
      child = instance_create( mod, dutname, 0, 0, FALSE, FALSE, FALSE, NULL );
      child->parent = parent;
      if( parent->child_head == NULL ) {
        parent->child_head       = child;
        parent->child_tail       = child;
      } else {
        parent->child_tail->next = child;
        parent->child_tail       = child;
      }
      db_list[curr_db]->leading_hierarchies = (char**)realloc_safe( db_list[curr_db]->leading_hierarchies, (sizeof( char* ) * db_list[curr_db]->leading_hier_num), (sizeof( char* ) * (db_list[curr_db]->leading_hier_num + 1)) );
      db_list[curr_db]->leading_hierarchies[db_list[curr_db]->leading_hier_num] = strdup_safe( lhier );
      db_list[curr_db]->leading_hier_num++;
    }
  }

}
 
/*!
 \param path Name of include path to search for `include directives.

 Adds the given include directory path to the include path list if the pathname exists.
*/
void search_add_include_path(
  const char* path
) { PROFILE(SEARCH_ADD_INCLUDE_PATH);

  char* tmp;  /* Temporary directory name */

  if( directory_exists( path ) ) {
    tmp = strdup_safe( path );
    (void)str_link_add( tmp, &inc_paths_head, &inc_paths_tail );
  } else {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Include directory %s does not exist", path );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );
  }

}

/*!
 \param path Name of directory to find unspecified Verilog files

 \throws anonymous directory_load

 Adds the given library directory path to the list if the pathname is valid.
*/
void search_add_directory_path(
  const char* path
) { PROFILE(SEARCH_ADD_DIRECTORY_PATH);

  if( directory_exists( path ) ) {
    /* If no library extensions have been specified, assume *.v */
    if( extensions_head == NULL ) {
      (void)str_link_add( strdup_safe( "v" ), &(extensions_head), &(extensions_tail) );
    }
    directory_load( path, extensions_head, &(use_files_head), &(use_files_tail) );
  } else {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Library directory %s does not exist", path );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );
  }

}

/*!
 \param file Name of Verilog file to add to scoring list.

 \throws anonymous Throw

 Adds the given file to the search path list if the file exists.
*/
void search_add_file(
  const char* file
) { PROFILE(SEARCH_ADD_FILE);

  char* tmp;  /* Temporary filename */

  if( file_exists( file ) ) {
    str_link* strl;
    if( (strl = str_link_find( file, use_files_head )) == NULL ) {
      tmp = strdup_safe( file );
      (void)str_link_add( tmp, &use_files_head, &use_files_tail );
    } else {
      strl->suppl = 0x0;
    }
  } else {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "File %s does not exist", file );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

}

/*!
 \param funit  Name of functional unit to specifically not score

 \throws anonymous Throw

 Checks the given functional unit name and adds this name to the list of functional units
 to exclude from coverage.
*/
void search_add_no_score_funit(
  const char* funit
) { PROFILE(SEARCH_ADD_NO_SCORE_FUNIT);

  char* tmp;  /* Temporary module name */

  if( is_func_unit( funit ) ) {
    tmp = strdup_safe( funit );
    (void)str_link_add( tmp, &no_score_head, &no_score_tail );
  } else {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Value of -e option (%s) is not a valid block name", funit );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

}

/*!
 \param ext_list String containing extensions to allow in search.

 \throws anonymous Throw Throw

 Parses the given +libext argument, extracting all extensions listed and storing them into
 the globally accessible extensions list.
*/
void search_add_extensions(
  const char* ext_list
) { PROFILE(SEARCH_ADD_EXTENSIONS);

  char        ext[30];               /* Holder for extension */
  int         ext_index = 0;         /* Index to ext array */
  const char* tmp       = ext_list;  /* Temporary extension name */

  assert( ext_list != NULL );

  while( *tmp != '\0' ) {
    assert( ext_index < 30 );
    if( *tmp == '+' ) { 
      ext[ext_index] = '\0';
      ext_index      = 0;
      (void)str_link_add( strdup_safe( ext ), &extensions_head, &extensions_tail );
    } else if( *tmp == '.' ) {
      if( ext_index > 0 ) {
        Throw 0;
      }
    } else {
      ext[ext_index] = *tmp;
      ext_index++;
    }
    tmp++;
  }

  /* If extension list is not empty, we have hit a parsing error */
  if( (strlen( tmp ) > 0) || (ext_index > 0) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Parsing error in +libext+%s  ", ext_list );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    gen_char_string( user_msg, ' ', (25 + (strlen( ext_list ) - strlen( tmp ))) );
    strcat( user_msg, "^" );
    print_output( user_msg, FATAL_WRAP, __FILE__, __LINE__ );
    Throw 0;
  }

}

/*!
 This function should be called after all parsing is completed.  
 Deletes all initialized lists.
*/
void search_free_lists() { PROFILE(SEARCH_FREE_LISTS);

  str_link_delete_list( inc_paths_head );
  str_link_delete_list( use_files_head );
  str_link_delete_list( extensions_head );
  str_link_delete_list( no_score_head );

  PROFILE_END;

}

