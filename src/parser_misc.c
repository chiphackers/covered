/*!
 \file     parser_misc.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/19/2001
*/

#include <stdio.h>

#include  "parser_misc.h"

extern const char*vl_file;
unsigned error_count = 0;
unsigned warn_count = 0;

void VLerror( char* msg ) {

  error_count += 1;
  fprintf( stderr, "%s\n", msg );

}

void yywarn( char* msg ) {

  warn_count += 1;
  fprintf( stderr, "%s\n", msg );

}

int VLwrap() {

  return -1;

}

