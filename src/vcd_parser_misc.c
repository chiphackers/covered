/*!
 \file     vcd_parser_misc.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/3/2001
*/

#include <stdio.h>

#include  "vcd_parser_misc.h"

unsigned vcd_error_count = 0;
unsigned vcd_warn_count = 0;

void VCDerror( char* msg ) {

  vcd_error_count += 1;
  fprintf( stderr, "%s\n", msg );

}

void yywarn( char* msg ) {

  vcd_warn_count += 1;
  fprintf( stderr, "%s\n", msg );

}

int VCDwrap() {

  return -1;

}

