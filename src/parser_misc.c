/*!
 \file     parser_misc.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/19/2001
*/

#include <stdio.h>

#include "parser_misc.h"
#include "util.h"

extern char user_msg[USER_MSG_LENGTH];

/*!
 Counts the number of errors found during the parsing process.
*/
unsigned error_count = 0;

/*!
 Counts the number of warnings found during the parsing process.
*/
unsigned warn_count  = 0;

/*!
 \param msg  String containing error message to display to user.

 Outputs specified error message string to standard output and increments
 error count.
*/
void VLerror( char* msg ) {

  error_count += 1;
  
  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d", msg, yylloc.text, yylloc.first_line );
  print_output( user_msg, FATAL, __FILE__, __LINE__ );

}

/*!
 \param msg  String containing warning message to display to user.

 Outputs specified warning message string to standard output and increments
 warning count.
*/
void VLwarn( char* msg ) {

  warn_count += 1;
  
  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d", msg, yylloc.text, yylloc.first_line );
  print_output( user_msg, WARNING, __FILE__, __LINE__ );

}

/*!
 Called by parser when file wrapping is required.
*/
int VLwrap() {

  return -1;

}

/*
 $Log$
 Revision 1.3  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.2  2002/12/01 06:37:52  phase1geo
 Adding appropriate error output and causing score command to proper exit
 if parser errors are found.

*/

