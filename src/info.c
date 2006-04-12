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
 \file     info.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/12/2003
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "info.h"
#include "util.h"


extern char** merge_in;
extern int    merge_in_num;
extern bool   flag_exclude_assign;
extern bool   flag_exclude_always;
extern bool   flag_exclude_initial;


/*!
 If this flag is set to a value of 1, it indicates that the current CDD file has
 been scored and is available for coverage reporting (if CDD file has not been scored,
 errors will occur when trying to get coverage numbers).
*/
bool flag_scored = FALSE;

/*!
 This string specifies the Verilog hierarchy leading up to the DUT.  This value is
 taken from the -i value (or is a value of '*' if the -t option is only specified).
*/
char** leading_hierarchies;

/*!
 Specifies the number of hierarchies stored in the leading_hierarchies array.
*/
int leading_hier_num;

/*!
 Set to TRUE if more than one leading hierarchy exists and it differs with the first leading hierarchy.
*/
bool leading_hiers_differ;

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
int cdd_version = CDD_VERSION;


/*!
 Initializes all variables used for information.
*/
void info_initialize() {

  leading_hier_num     = 0;
  leading_hiers_differ = FALSE;

}

/*!
 \param file  Pointer to file to write information to.
 
 Writes information line to specified file.
*/
void info_db_write( FILE* file ) {

  int i;  /* Loop iterator */

  assert( leading_hier_num > 0 );

  fprintf( file, "%d %d %s %d %d %d %d %d",
           DB_TYPE_INFO,
           flag_scored,
           leading_hierarchies[0],
           CDD_VERSION,
           flag_exclude_assign,
           flag_exclude_always,
           flag_exclude_initial,
           merge_in_num );

  /* Display any merge filename information */
  if( leading_hier_num == merge_in_num ) {
    for( i=0; i<merge_in_num; i++ ) {
      fprintf( file, " %s %s", merge_in[i], leading_hierarchies[i] );
    }
  } else {
    assert( (leading_hier_num - 1) == merge_in_num );
    for( i=0; i<merge_in_num; i++ ) {
      fprintf( file, " %s %s", merge_in[i], leading_hierarchies[i+1] );
    }
  }

  fprintf( file, "\n" );

}

/*!
 \param line  Pointer to string containing information line to parse.

 Reads information line from specified string and stores its information.
*/
bool info_db_read( char** line ) {

  bool retval = TRUE;  /* Return value for this function */
  int  chars_read;     /* Number of characters scanned in from this line */
  int  scored;         /* Indicates if this file contains scored data */
  int  version;        /* Contains CDD version from file */
  int  mnum;           /* Temporary merge num */
  char tmp1[4096];     /* Temporary string */
  char tmp2[4096];     /* Temporary string */
  int  i;              /* Loop iterator */

  if( sscanf( *line, "%d %s %d %d %d %d %d%n", &scored, tmp1, &version, &flag_exclude_assign, &flag_exclude_always,
                                               &flag_exclude_initial, &mnum, &chars_read ) == 7 ) {

    *line = *line + chars_read;

    if( version != CDD_VERSION ) {
      print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
      retval = FALSE;
    }

    /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
    if( (leading_hier_num > 0) && (strcmp( leading_hierarchies[0], tmp1 ) != 0) ) {
      leading_hiers_differ = TRUE;
    }

    /* Assign this hierarchy to the leading hierarchies array */
    leading_hierarchies = (char**)realloc( leading_hierarchies, (sizeof( char* ) * (leading_hier_num + 1)) );
    leading_hierarchies[leading_hier_num] = strdup_safe( tmp1, __FILE__, __LINE__ );
    leading_hier_num++;

    for( i=0; i<mnum; i++ ) {

      if( sscanf( *line, "%s %s%n", tmp1, tmp2, &chars_read ) == 2 ) {

        *line = *line + chars_read;

        /* Add merged file */
        merge_in = (char**)realloc( merge_in, (sizeof( char* ) * (merge_in_num + 1)) );
        merge_in[merge_in_num] = strdup_safe( tmp1, __FILE__, __LINE__ );
        merge_in_num++;

        /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
        if( strcmp( leading_hierarchies[0], tmp2 ) != 0 ) {
          leading_hiers_differ = TRUE;
        }

        /* Add its hierarchy */
        leading_hierarchies = (char**)realloc( leading_hierarchies, (sizeof( char* ) * (leading_hier_num + 1)) );
        leading_hierarchies[leading_hier_num] = strdup_safe( tmp2, __FILE__, __LINE__ );
        leading_hier_num++;

      } else {
        print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    }

    /* Set scored flag */
    flag_scored = (scored == TRUE) ? TRUE : flag_scored;

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  return( retval );

}


/*
 $Log$
 Revision 1.12  2006/04/12 13:28:37  phase1geo
 Fixing problem with memory allocation for merged files.

 Revision 1.11  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.10  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.9  2005/12/12 03:46:14  phase1geo
 Adding exclusion to score command to improve performance.  Updated regression
 which now fully passes.

 Revision 1.8  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.7  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.6  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.5  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.4  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.3  2003/10/17 02:12:38  phase1geo
 Adding CDD version information to info line of CDD file.  Updating regression
 for this change.

 Revision 1.2  2003/02/18 20:17:02  phase1geo
 Making use of scored flag in CDD file.  Causing report command to exit early
 if it is working on a CDD file which has not been scored.  Updated testsuite
 for these changes.

 Revision 1.1  2003/02/12 14:56:26  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

*/

