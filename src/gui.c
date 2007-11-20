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
 \file     gui.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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
#include "func_unit.h"
#include "ovl.h"


extern funit_link* funit_head;
extern isuppl      info_suppl;


/*!
 \param funit_names  Pointer to array containing functional unit names
 \param funit_types  Pointer to array containing functional unit types
 \param funit_size   Pointer to integer containing size of functional unit array.

 \return Returns TRUE if function is successful; otherwise, returns FALSE.

 Creates an array of the functional unit names/types that exist in the design.
*/
bool funit_get_list( char*** funit_names, char*** funit_types, int* funit_size ) {

  bool        retval = TRUE;  /* Return value for this function */
  funit_link* curr;           /* Pointer to current functional unit link in list */
  int         i;              /* Index to module list */
  char        tmpstr[10];     /* Temporary string */
  char        fname[4096];    /* Temporary storage for functional unit name */

  /* Initialize functional unit array size */
  *funit_size = 0;

  /* Count the number of functional units */
  curr = funit_head;
  while( curr != NULL ) {
    if( !funit_is_unnamed( curr->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( funit_get_curr_module( curr->funit ) )) ) {
      (*funit_size)++;
    }
    curr = curr->next;
  }

  /* If we have any functional units in the currently loaded design, create the list now */
  if( *funit_size > 0 ) {

    /* Allocate array to store functional unit names */
    *funit_names = (char**)malloc_safe( (sizeof( char* ) * (*funit_size)), __FILE__, __LINE__ );
    *funit_types = (char**)malloc_safe( (sizeof( char* ) * (*funit_size)), __FILE__, __LINE__ );

    /* Now let's populate the functional unit list */
    i    = 0;
    curr = funit_head;
    while( curr != NULL ) {
      if( !funit_is_unnamed( curr->funit ) &&
          ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( funit_get_curr_module( curr->funit ) )) ) {
        (*funit_names)[i] = strdup_safe( curr->funit->name, __FILE__, __LINE__ );
        snprintf( tmpstr, 10, "%d", curr->funit->type );
        (*funit_types)[i] = strdup_safe( tmpstr, __FILE__, __LINE__ );
        i++;
      }
      curr = curr->next;
    }

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit to get filename for.
 \param funit_type  Type of functional unit to get filename for.

 \return Returns name of filename containing specified funit_name if functional unit name was found in
         design; otherwise, returns a value of NULL.

 Searches design for functional unit named funit_name.  If functional unit is found, the filename of the
 functional unit is returned to the calling function.  If the functional unit was not found, a value of NULL
 is returned to the calling function indicating an error occurred.
*/
char* funit_get_filename( const char* funit_name, int funit_type ) {

  func_unit   funit;         /* Temporary functional unit container used for searching             */
  funit_link* funitl;        /* Pointer to functional unit link containing matched functional unit */
  char*       fname = NULL;  /* Name of filename containing specified functional unit              */

  funit.name = strdup_safe( funit_name, __FILE__, __LINE__ );
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {
     
    fname = strdup_safe( funitl->funit->filename, __FILE__, __LINE__ );

  }

  free_safe( funit.name );

  return( fname );

}

/*!
 \param funit_name  Name of functional unit to get start and end line numbers for.
 \param funit_type  Type of functional unit to get start and end line numbers for.
 \param start_line  Pointer to value that will contain starting line number of this functional unit.
 \param end_line    Pointer to value that will contain ending line number of this functional unit.

 \return Returns a value of TRUE if functional unit was found; otherwise, returns a value of FALSE.

 Finds specified functional unit name in design and returns the starting and ending line numbers of
 the found functional unit, returning a value of TRUE to the calling function.  If the functional unit was
 not found in the design, a value of FALSE is returned.
*/
bool funit_get_start_and_end_lines( const char* funit_name, int funit_type, int* start_line, int* end_line ) {

  bool        retval = TRUE;  /* Return value of this function                                      */
  func_unit   funit;          /* Temporary functional unit container used for searching             */
  funit_link* funitl;         /* Pointer to functional unit line containing matched functional unit */
  
  funit.name = strdup_safe( funit_name, __FILE__, __LINE__ );
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    *start_line = funitl->funit->start_line;
    *end_line   = funitl->funit->end_line;

  } else {

    retval = FALSE;

  }

  free_safe( funit.name );

  return( retval );

}

/*
 $Log$
 Revision 1.9  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.8  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.7  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.6  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.5  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.4  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

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

