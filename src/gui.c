/*!
 \file     gui.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/24/2003
*/

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

  /* Allocate array to store module names */
  *mod_list = (char**)malloc_safe( sizeof( char* ) * (*mod_size) );

  /* Now let's populate the module list */
  i    = 0;
  curr = mod_head;
  while( curr != NULL ) {
    (*mod_list)[i] = strdup( curr->mod->name );
    i++;
    curr = curr->next;
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
char* module_get_filename( char* mod_name ) {

  module    mod;           /* Temporary module container used for searching    */
  mod_link* modl;          /* Pointer to module link containing matched module */
  char*     fname = NULL;  /* Name of filename containing specified module     */

  mod.name = mod_name;

  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {
     
    fname = strdup( modl->mod->filename );

  }

  return( fname );

}

/*
 $Log$
*/

