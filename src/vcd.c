/*!
 \file     vcd.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/21/2002
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "vcd.h"
#include "db.h"
#include "util.h"


extern char user_msg[USER_MSG_LENGTH];


/*!
 \param vcd  File handle pointer to opened VCD file.

 Parses specified file until $end keyword is seen, ignoring all text inbetween.
*/
void vcd_parse_def_ignore( FILE* vcd ) {

  bool end_seen = FALSE;     /* If set to true, $end keyword was seen */
  char token[256];           /* String value of current token         */
  int  tokval;               /* Set to number of tokens found         */

  while( !end_seen && ((tokval = fscanf( vcd, "%s", token )) == 1) ) {
    if( strncmp( "$end", token, 4 ) == 0 ) {
      end_seen = TRUE;
    }
  }

  assert( end_seen );

}

/*!
 \param vcd  File handle pointer to opened VCD file.

 Parses definition $var keyword line until $end keyword is seen.
*/
void vcd_parse_def_var( FILE* vcd ) {

  char type[256];        /* Variable type                   */
  int  size;             /* Bit width of specified variable */
  char id_code[256];     /* Unique variable identifier_code */
  char ref[256];         /* Name of variable in design      */
  char tmp[15];          /* Temporary string holder         */

  if( fscanf( vcd, "%s %d %s %s %s", type, &size, id_code, ref, tmp ) == 5 ) {

    if( strncmp( "$end", tmp, 4 ) != 0 ) {

      if( (fscanf( vcd, "%s", tmp ) != 1) || (strncmp( "$end", tmp, 4 ) != 0) ) {
        print_output( "Unrecognized $var format", FATAL );
        exit( 1 );
      }

    }

    /* For now we will let any type and size slide */
    db_assign_symbol( ref, id_code );
    
  } else {

    print_output( "Unrecognized $var format", FATAL );
    exit( 1 );
  
  }

}

/*!
 \param vcd  File handle pointer to opened VCD file.

 Parses definition $scope keyword line until $end keyword is seen.
*/
void vcd_parse_def_scope( FILE* vcd ) {

  char type[256];     /* Scope type                 */
  char id[256];       /* Name of scope to change to */

  if( fscanf( vcd, "%s %s $end", type, id ) == 2 ) {

    /* For now we will let any type slide */
    db_set_vcd_scope( id );

  } else {

    print_output( "Unrecognized $scope format", FATAL );
    exit( 1 );

  }
  
}

/*!
 \param vcd  File handle pointer to opened VCD file.

 Parses all definition information from specified file.
*/
void vcd_parse_def( FILE* vcd ) {

  bool enddef_found = FALSE;  /* If set to true, definition section is finished */
  char keyword[256];          /* Holds keyword value                            */

  while( !enddef_found && (fscanf( vcd, "%s", keyword ) == 1) ) {

    if( keyword[0] == '$' ) {

      if( strncmp( "var", (keyword + 1), 3 ) == 0 ) {
        vcd_parse_def_var( vcd );
      } else if( strncmp( "scope", (keyword + 1), 5 ) == 0 ) {
        vcd_parse_def_scope( vcd );
      } else if( strncmp( "upscope", (keyword + 1), 7 ) == 0 ) {
        db_vcd_upscope();
        vcd_parse_def_ignore( vcd );
      } else if( strncmp( "enddefinitions", (keyword + 1), 14 ) == 0 ) {
        enddef_found = TRUE;
        vcd_parse_def_ignore( vcd );
      } else {
        vcd_parse_def_ignore( vcd );
      }

    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Non-keyword located where one should have been \"%s\"", keyword );
      print_output( user_msg, FATAL );
      exit( 1 );

    }
  
  }

  assert( enddef_found );

}

/*!
 \param vcd    File handle of opened VCD file.
 \param value  String containing value of current signal.

 Reads the next token from the file and calls the appropriate database storage
 function for this signal change.
*/
void vcd_parse_sim_vector( FILE* vcd, char* value ) {

  char sym[256];      /* String value of signal symbol */

  if( fscanf( vcd, "%s", sym ) == 1 ) {

    db_set_symbol_string( sym, value );

  } else {

    print_output( "Bad file format", FATAL );
    exit( 1 );

  }

}

/*!
 \param vcd  File handle of opened VCD file.

 Reads in symbol from simulation vector line that is to be ignored 
 (unused).  Signals an error message if the line is improperly formatted.
*/
void vcd_parse_sim_ignore( FILE* vcd ) {

  char sym[256];      /* String value of signal symbol */

  if( fscanf( vcd, "%s", sym ) != 1 ) {

    print_output( "Bad file format", FATAL );
    exit( 1 );

  }

}

/*!
 \param vcd  File handle of opened VCD file.

 Parses all lines that occur in the simulation portion of the VCD file.
*/
void vcd_parse_sim( FILE* vcd ) {

  char token[4100];         /* Current token from VCD file       */
  int  last_timestep = -1;  /* Value of last timestamp from file */
 
  while( !feof( vcd ) && (fscanf( vcd, "%s", token ) == 1) ) {

    if( token[0] == '$' ) {

      /* Currently ignore all simulation keywords */

    } else if( (token[0] == 'b') || (token[0] == 'B') ) {

      vcd_parse_sim_vector( vcd, (token + 1) );

    } else if( (token[0] == 'r') || (token[0] == 'B') ) {

      vcd_parse_sim_ignore( vcd );

    } else if( token[0] == '#' ) {

      if( last_timestep >= 0 ) {
        db_do_timestep( last_timestep );
      }
      last_timestep = atol( token + 1 );

    } else if( (token[0] == '0') ||
               (token[0] == '1') ||
               (token[0] == 'x') ||
               (token[0] == 'X') ||
               (token[0] == 'z') ||
               (token[0] == 'Z') ) {

      db_set_symbol_char( token + 1, token[0] );

    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Badly placed token \"%s\"", token );
      print_output( user_msg, FATAL );
      exit( 1 );

    }

  }

}

/*!
 \param vcd_file  Name of VCD file to parse.

 Reads specified VCD file for relevant information and calls the database
 functions when appropriate to store this information.  This replaces the
 need for a lexer and parser which should increase performance.
*/
void vcd_parse( char* vcd_file ) {

  FILE* vcd_handle;        /* Pointer to opened VCD file */

  if( (vcd_handle = fopen( vcd_file, "r" )) != NULL ) {

    vcd_parse_def( vcd_handle );
    vcd_parse_sim( vcd_handle );

  } else {

    print_output( "Unable to open specified VCD file", FATAL );
    exit( 1 );

  }

}

/* $Log$
/* Revision 1.3  2002/10/11 05:23:21  phase1geo
/* Removing local user message allocation and replacing with global to help
/* with memory efficiency.
/*
/* Revision 1.2  2002/09/18 22:19:25  phase1geo
/* Adding handler for option bit select in $var line.
/*
/* Revision 1.1  2002/07/22 05:24:46  phase1geo
/* Creating new VCD parser.  This should have performance benefits as well as
/* have the ability to handle any problems that come up in parsing.
/* */
