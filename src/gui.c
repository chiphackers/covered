/*!
 \file     gui.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/24/2003
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include "string.h"
#endif

#include "gui.h"
#include "util.h"
#include "link.h"


extern mod_link* mod_head;


/*!
 \param mod_list  Pointer to array containing module names
 \param mod_size  Pointer to integer containing size of module array.

 \return Returns TRUE if function is successful; otherwise, returns FALSE.

 Creates an array of the module names that exist in the design.
*/
bool module_get_list( char*** mod_list, int* mod_size ) {

  bool      retval = TRUE;  /* Return value for this function         */
  mod_link* curr;           /* Pointer to current module link in list */
  int       i;              /* Index to module list                   */

  /* Initialize module array size */
  *mod_size = 0;

  /* Count the number of modules */
  curr = mod_head;
  while( curr != NULL ) {
    (*mod_size)++;
    curr = curr->next;
  }

  /* If we have any modules in the currently loaded design, create the list now */
  if( *mod_size > 0 ) {

    /* Allocate array to store module names */
    *mod_list = (char**)malloc_safe( (sizeof( char* ) * (*mod_size)), __FILE__, __LINE__ );

    /* Now let's populate the module list */
    i    = 0;
    curr = mod_head;
    while( curr != NULL ) {
      (*mod_list)[i] = strdup_safe( curr->mod->name, __FILE__, __LINE__ );
      i++;
      curr = curr->next;
    }

  }

  return( retval );

}

/*!
 \param mod_name  Name of module to get filename for.

 \return Returns name of filename containing specified mod_name if module name was found in
         design; otherwise, returns a value of NULL.

 Searches design for module named mod_name.  If module is found, the filename of the
 module is returned to the calling function.  If the module was not found, a value of NULL
 is returned to the calling function indicating an error occurred.
*/
char* module_get_filename( const char* mod_name ) {

  module    mod;           /* Temporary module container used for searching    */
  mod_link* modl;          /* Pointer to module link containing matched module */
  char*     fname = NULL;  /* Name of filename containing specified module     */

  mod.name = strdup_safe( mod_name, __FILE__, __LINE__ );

  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {
     
    fname = strdup_safe( modl->mod->filename, __FILE__, __LINE__ );

  }

  free_safe( mod.name );

  return( fname );

}

/*!
 \param mod_name    Name of module to get start and end line numbers for.
 \param start_line  Pointer to value that will contain starting line number of this module.
 \param end_line    Pointer to value that will contain ending line number of this module.

 \return Returns a value of TRUE if module was found; otherwise, returns a value of FALSE.

 Finds specified module name in design and returns the starting and ending line numbers of
 the found module, returning a value of TRUE to the calling function.  If the module was
 not found in the design, a value of FALSE is returned.
*/
bool module_get_start_and_end_lines( const char* mod_name, int* start_line, int* end_line ) {

  bool      retval = TRUE;  /* Return value of this function                    */
  module    mod;            /* Temporary module container used for searching    */
  mod_link* modl;           /* Pointer to module line containing matched module */
  
  mod.name = strdup_safe( mod_name, __FILE__, __LINE__ );

  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {

    *start_line = modl->mod->start_line;
    *end_line   = modl->mod->end_line;

  } else {

    retval = FALSE;

  }

  free_safe( mod.name );

  return( retval );

}

/*
 $Log$
 Revision 1.3  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.2  2003/11/29 06:55:48  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.1  2003/11/24 17:48:56  phase1geo
 Adding gui.c/.h files for functions related to the GUI interface.  Updated
 Makefile.am for the inclusion of these files.

*/

