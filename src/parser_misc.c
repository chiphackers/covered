/*!
 \file     parser_misc.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/19/2001
*/

#include <stdio.h>

#include "parser_misc.h"
#include "util.h"

extern const char* vl_file;
extern char        user_msg[USER_MSG_LENGTH];

unsigned error_count = 0;
unsigned warn_count  = 0;

void VLerror( char* msg ) {

  error_count += 1;
  
  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d", msg, yylloc.text, yylloc.first_line );
  print_output( user_msg, FATAL );

}

void VLwarn( char* msg ) {

  warn_count += 1;
  
  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d", msg, yylloc.text, yylloc.first_line );
  print_output( user_msg, WARNING );

}

int VLwrap() {

  return -1;

}

/*
 $Log$
*/

