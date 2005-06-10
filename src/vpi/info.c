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


extern char* merge_in0;
extern char* merge_in1;

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
char leading_hierarchy[4096];

/*!
 If we have merged and the leading hierarchy of the merged CDDs don't match, the value
 contains the leading hierarchy of the other merged CDD file.  This value will be used
 in the SPECIAL NOTES section of the report to indicate to the user that this case
 occurred and that the leading hierarchy value should not be used in the report output.
*/
char second_hierarchy[4096];

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
int  cdd_version = CDD_VERSION;

/*!
 If this value is set to INFO_ONE_MERGED or INFO_TWO_MERGED, specifies that this CDD file
 was generated from a merge command (not from a score command).
*/
int merged_code = INFO_NOT_MERGED;


/*!
 Initializes all variables used for information.
*/
void info_initialize() {

  leading_hierarchy[0] = '\0';
  second_hierarchy[0]  = '\0';

}

/*!
 \param file  Pointer to file to write information to.
 
 Writes information line to specified file.
*/
void info_db_write( FILE* file ) {

  fprintf( file, "%d %d %s %d %d",
           DB_TYPE_INFO,
           flag_scored,
           leading_hierarchy,
           CDD_VERSION,
           merged_code );

  switch( merged_code ) {
    case INFO_NOT_MERGED :
      fprintf( file, "\n" );
      break;
    case INFO_ONE_MERGED :
      assert( merge_in0 != NULL );
      fprintf( file, " %s %s\n", merge_in0, second_hierarchy );
      break;
    case INFO_TWO_MERGED :
      assert( merge_in0 != NULL );
      assert( merge_in1 != NULL );
      fprintf( file, " %s %s %s\n", merge_in0, merge_in1, second_hierarchy );
      break;
    default :  break;
  }

}

/*!
 \param line  Pointer to string containing information line to parse.

 Reads information line from specified string and stores its information.
*/
bool info_db_read( char** line ) {

  bool retval = TRUE;  /* Return value for this function                 */
  int  chars_read;     /* Number of characters scanned in from this line */
  int  scored;         /* Indicates if this file contains scored data    */
  int  version;        /* Contains CDD version from file                 */
  int  mcode;          /* Temporary merge code                           */
  char tmp[4096];      /* Temporary string                               */

  if( sscanf( *line, "%d %s %d %d%n", &scored, tmp, &version, &mcode, &chars_read ) == 4 ) {

    *line = *line + chars_read;

    if( version != CDD_VERSION ) {
      print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
      retval = FALSE;
    }

    /* If we have not assigned leading hierarchy yet, do it now; otherwise, assign second hierarchy */
    if( leading_hierarchy[0] == '\0' ) {
      strcpy( leading_hierarchy, tmp );
    } else {
      strcpy( second_hierarchy, tmp );
    }

    if( mcode != INFO_NOT_MERGED ) {
      if( sscanf( *line, "%s%n", tmp, &chars_read ) == 1 ) {
        if( merge_in0 == NULL ) {
          merge_in0 = strdup_safe( tmp, __FILE__, __LINE__ );
        }
        *line = *line + chars_read;
        if( mcode == INFO_TWO_MERGED ) {
          if( sscanf( *line, "%s%n", tmp, &chars_read ) == 1 ) {
            if( merge_in1 == NULL ) {
              merge_in1 = strdup_safe( tmp, __FILE__, __LINE__ );
            }
            *line = *line + chars_read;
          } else {
            print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
            retval = FALSE;
          }
        }
        if( sscanf( *line, "%s%n", tmp, &chars_read ) == 1 ) {
          if( second_hierarchy[0] == '\0' ) {
            strcpy( second_hierarchy, tmp );
          }
          *line = *line + chars_read;
        } else {
          print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
          retval = FALSE;
        }
      } else {
        print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }
    }

    flag_scored = (scored == TRUE) ? TRUE : flag_scored;
    merged_code = (merged_code == INFO_NOT_MERGED) ? mcode : merged_code;

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  return( retval );

}


/*
 $Log$
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

