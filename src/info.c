/*!
 \file     info.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/12/2003
*/


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "info.h"


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
 \param file  Pointer to file to write information to.
 
 Writes information line to specified file.
*/
void info_db_write( FILE* file ) {

  fprintf( file, "%d %d %s\n",
           DB_TYPE_INFO,
           flag_scored,
           leading_hierarchy );

}

/*!
 \param line  Pointer to string containing information line to parse.

 Reads information line from specified string and stores its information.
*/
bool info_db_read( char** line ) {

  bool retval = TRUE;  /* Return value for this function                 */
  int  chars_read;     /* Number of characters scanned in from this line */

  if( sscanf( *line, "%d %s%n", &flag_scored, leading_hierarchy, &chars_read ) == 2 ) {

    *line = *line + chars_read;

  } else {

    retval = FALSE;

  }

  return( retval );

}


/*
 $Log$
*/

