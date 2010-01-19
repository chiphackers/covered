/*
 Copyright (c) 2006-2010 Trevor Williams

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
 \file     vcd.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     7/21/2002
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "vcd.new.h"
#include "db.h"
#include "util.h"
#include "symtable.h"


/*!
 Specifies the byte size of the VCD read buffer.
*/
#define VCD_BUFSIZE    32768

/*!
 Reads the next character from the VCD file.
*/
#define vcd_getch(vcd) ((vcd_rdbuf_cur != vcd_rdbuf_end) ? ((int)(*(vcd_rdbuf_cur++))) : vcd_getch_fetch( vcd ))

/*!
 Reads in a token that will be discarded.  Breaks the main while loop if an $end or an EOF is encountered.
*/
#define vcd_get(vcd)   tok = vcd_get_token1( vcd, 0 );  if( (tok==T_END) || (tok==T_EOF) ) { break; }

/*!
 Returns a token in yytext starting at byte 0.
*/
#define vcd_get_token(vcd) vcd_get_token1( vcd, 0 )

/*!
 Adds a token to the existing yytext.
*/
#define vcd_append_token(vcd, new_start) vcd_get_token1( vcd, (new_start = (vcd_yylen + 1)) )

extern char       user_msg[USER_MSG_LENGTH];
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;
extern char**     curr_inst_scope;
extern int        curr_inst_scope_size;

/*!
 Pointer to start of VCD read buffer.
*/
static char* vcd_rdbuf_start = NULL;

/*!
 Pointer to end of VCD read buffer.
*/
static char* vcd_rdbuf_end = NULL;

/*!
 Pointer to currently read character from read buffer.
*/
static char* vcd_rdbuf_cur = NULL;

/*!
 Contains the string version of the next read token.
*/
static char* vcd_yytext = NULL;

/*!
 Number of bytes allocated for the vcd_yytext array.
*/
static int vcd_yytext_size = 0;

/*!
 Contains the length of the read token.
*/
static int vcd_yylen = 0;


/*!
 \return Returns a 32-bit value containing the index into the symbol array based off of the
         VCD symbol.
*/
int vcd_calc_index(
  char* sym
) { PROFILE(VCD_CALC_INDEX);

  int slen  = strlen( sym );
  int i;
  int index = 0;

  for( i=0; i<slen; i++ ) {
    index = (index * 94) + ((int)sym[i] - 33);
  }

  PROFILE_END;

  return( index );

}

/*!
 Reads up to the next 32 Kb from the dumpfile, adjusts the buffer pointers and returns the next character.
*/
static int vcd_getch_fetch(
  FILE* vcd  /*!< Pointer to VCD file to read */
) { PROFILE(VCD_GETCH_FETCH);

  int    ch = -1;  /* Return value for this function */
  size_t rd;

  errno = 0;

  if( !feof( vcd ) ) {

    rd = fread( vcd_rdbuf_start, sizeof( char ), VCD_BUFSIZE, vcd );
    vcd_rdbuf_end = (vcd_rdbuf_cur = vcd_rdbuf_start) + rd;

    if( rd && (errno == 0) ) {
      ch = (int)(*(vcd_rdbuf_cur++));
    }

  }

  PROFILE_END;

  return( ch );

}

/*!
 \return Returns the next token read from the VCD buffer.

 Reads in the next token, storing the result in vcd_yytext and vcd_yylen.
*/
static int vcd_get_token1(
  FILE* vcd,   /*!< Pointer to dumpfile to read */
  int   start  /*!< Index of yytext to start adding characters to */
) { PROFILE(VCD_GET_TOKEN);

  int   token     = T_UNKNOWN;  /* Return value for this function */
  int   ch;
  int   i;
  int   len       = start;
  int   is_string = 0;
  char* yyshadow;

  /* Read whitespace */
  for(;;) {
    ch = vcd_getch( vcd );
    if( ch < 0 ) {
      token = T_EOF;
      break;
    }
    if( ch > ' ' ) {
      break;
    }
  }

  if( token != T_EOF ) {

    if( ch != '$' ) {
      is_string = 1;
    }

    for( vcd_yytext[len++] = ch; ; vcd_yytext[len++] = ch ) {
      if( len == vcd_yytext_size ) {
        vcd_yytext      = (char*)realloc_safe_nolimit( vcd_yytext, vcd_yytext_size, ((vcd_yytext_size * 2) + 1) );
        vcd_yytext_size = ((vcd_yytext_size * 2) + 1);
      }
      ch = vcd_getch( vcd );
      if( ch <= ' ' ) {
        break;
      }
    }
    vcd_yytext[len] = '\0';
    vcd_yylen       = len;

    if( is_string ) {
      token = T_STRING;
    } else {
      token = vcd_keyword_code( (vcd_yytext + 1), (len - 1) );
    }

  }

  PROFILE_END;

  return( token );

}

/*!
 Reads the line to the $end token, ignoring the contents of the body.
*/
static void vcd_sync_end(
  FILE* vcd  /*!< Pointer to VCD file */
) { PROFILE(VCD_SYNC_END);

  int tok;

  for(;;) {
    tok = vcd_get_token( vcd );
    if( (tok==T_END) || (tok==T_EOF) ) {
      break;
    }
  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw Throw

 Parses definition $var keyword line until $end keyword is seen.
*/
static void vcd_parse_def_var(
  FILE* vcd  /*!< Pointer to VCD file to read */
) { PROFILE(VCD_PARSE_DEF_VAR);

  int  size;
  char id_code[256];        /* Unique variable identifier_code */
  char ref[256];            /* Name of variable in design */
  char reftmp[256];         /* Temporary variable name */
  char tmp[256];            /* Temporary string holder */
  int  msb        = -1;     /* Most significant bit */
  int  lsb        = -1;     /* Least significant bit */
  int  tmplsb;              /* Temporary LSB if swapping is needed */
  bool found_real = FALSE;

  int tok;

  Try {

    /* Get the value type */
    if( vcd_get_token( vcd ) != T_STRING ) { Throw 0; }
    if( strncmp( "real", vcd_yytext, 4 ) == 0 ) { found_real = TRUE; }

    /* Get the size */
    if( vcd_get_token( vcd ) != T_STRING ) { Throw 0; }
    size = atoi( vcd_yytext );
      
    /* Get the ID code */
    if( vcd_get_token( vcd ) == T_EOF ) { Throw 0; }
    assert( vcd_yylen <= 256 );
    strcpy( id_code, vcd_yytext );
     
    /* Get the ref */
    if( (tok = vcd_get_token( vcd )) != T_STRING ) { Throw 0; }
    assert( vcd_yylen <= 256 );
    strcpy( ref, vcd_yytext );

    /* Get the tmp */
    tok = vcd_get_token( vcd );

    if( found_real ) {
  
      msb = 63;
      lsb = 0;

    } else {

      if( tok != T_END ) {

        if( tok != T_STRING ) { Throw 0; }
        if( sscanf( vcd_yytext, "\[%d:%d]", &msb, &lsb ) != 2 ) {
          if( sscanf( vcd_yytext, "\[%d]", &lsb ) != 1 ) { Throw 0; }
          msb = lsb;
        }
        if( vcd_get_token( vcd ) != T_END ) { Throw 0; }

      } else if( sscanf( ref, "%[a-zA-Z0-9_]\[%s].", reftmp, tmp ) == 2 ) {

        /* This is a hierarchical reference so we shouldn't modify ref -- quirky behavior from VCS */
        msb = size - 1;
        lsb = 0;

      } else if( sscanf( ref, "%[a-zA-Z0-9_]\[%s]", reftmp, tmp ) == 2 ) {
 
        strcpy( ref, reftmp );

        if( sscanf( tmp, "%d:%d", &msb, &lsb ) != 2 ) {
          if( sscanf( tmp, "%d", &lsb ) != 1 ) { Throw 0; }
          msb = lsb;
        }

      } else {

        msb = size - 1;
        lsb = 0;

      }

    }

  } Catch_anonymous {

    print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
    Throw 0;
  
  }

  /* For now we will let any type and size slide */
  db_assign_symbol( ref, id_code, msb, lsb );

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw vcd_parse_def_scope vcd_parse_def_var

 Parses all definition information from specified file.
*/
static void vcd_parse_def(
  FILE* vcd  /*!< File handle pointer to opened VCD file */
) { PROFILE(VCD_PARSE_DEF);

  bool         enddef_found = FALSE;  /* If set to true, definition section is finished */
  char         keyword[256];          /* Holds keyword value */
  int          chars_read;            /* Number of characters scanned in */
  char*        line         = NULL;
  unsigned int line_size;
  char*        ptr;
  bool         ignore       = FALSE;

  int tok = T_UNKNOWN;

  Try {

    for(;;) {

      switch( tok = vcd_get_token( vcd ) ) {
        case T_COMMENT   :
        case T_DATE      :
        case T_VERSION   :
        case T_TIMESCALE :
          vcd_sync_end( vcd );
          break;
        case T_SCOPE     :
          vcd_get( vcd );
          vcd_get( vcd );
          if( tok == T_STRING ) {
            db_set_vcd_scope( vcd_yytext );
          }
          vcd_sync_end( vcd );
          break;
        case T_UPSCOPE   :
          db_vcd_upscope();
          vcd_sync_end( vcd );
          break;
        case T_VAR       :
          vcd_parse_def_var( vcd );
          break;
        case T_ENDDEF    :
          goto end_definition;
        default          :
          {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown VCD definition token (%s)", vcd_yytext );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
      }

    }

  } Catch_anonymous {

    /* If we threw an exception, the curr_inst_scope array may be left in an allocated state */
    if( curr_inst_scope_size > 0 ) {
      int i;
      for( i=0; i<curr_inst_scope_size; i++ ) {
        free_safe( curr_inst_scope[i], (strlen( curr_inst_scope[i] ) + 1) );
      }
      free_safe( curr_inst_scope, (sizeof( char* ) * curr_inst_scope_size) );
      curr_inst_scope      = NULL;
      curr_inst_scope_size = 0;
    }

    Throw 0;

  }

  end_definition:

  if( tok != T_ENDDEF ) {
    print_output( "Specified VCD file is not a valid VCD file", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Check to see that at least one instance was found */
  db_check_dumpfile_scopes();

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads the next token from the file and calls the appropriate database storage
 function for this signal change.
*/
static void vcd_parse_sim_vector(
  FILE* vcd  /*!< File handle of opened VCD file */
) { PROFILE(VCD_PARSE_SIM_VECTOR);

  int tok;
  int sym_start;

  if( vcd_append_token( vcd, sym_start ) == T_EOF ) { Throw 0; }

  db_set_symbol_string( (vcd_yytext + sym_start), (vcd_yytext + 1) );

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads the next token from the file and calls the appropriate database storage
 function for this signal change.
*/
static void vcd_parse_sim_real(
  FILE* vcd  /*!< File handle of opened VCD file */
) { PROFILE(VCD_PARSE_SIM_REAL);

  int tok;
  int sym_start;

  if( vcd_append_token( vcd, sym_start ) == T_EOF ) { Throw 0; }

  db_set_symbol_string( (vcd_yytext + sym_start), (vcd_yytext + 1) );

  PROFILE_END;

}

/*!
 \throws anonymous db_do_timestep db_do_timestep vcd_parse_sim_vector Throw vcd_parse_sim_ignore

 Parses all lines that occur in the simulation portion of the VCD file.
*/
static void vcd_parse_sim(
  FILE* vcd  /*!< File handle of opened VCD file */
) { PROFILE(VCD_PARSE_SIM);

  uint64 last_timestep     = 0;      /* Value of last timestamp from file */
  bool   use_last_timestep = FALSE;  /* Specifies if timestep has been encountered */
  bool   simulate          = TRUE;   /* Specifies if we should continue to simulate */
 
  int tok;

  for(;;) {

    /* We ignore all other tokens besides value changes */
    if( (tok = vcd_get_token( vcd )) == T_STRING ) {

      switch( vcd_yytext[0] ) {
        case 'b' :
        case 'B' :
          vcd_parse_sim_vector( vcd );
          break;
        case 'r' :
        case 'R' :
          vcd_parse_sim_real( vcd );
          break;
        case '#' :
          if( use_last_timestep ) {
            simulate = db_do_timestep( last_timestep, FALSE );
          }
          last_timestep = ato64( vcd_yytext + 1 );
          use_last_timestep = TRUE;
          break;
        case '0' :
        case '1' :
        case 'x' :
        case 'X' :
        case 'z' :
        case 'Z' :
          db_set_symbol_char( (vcd_yytext + 1), vcd_yytext[0] );
          break;
        default  :
          {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Badly placed token \"%s\"", vcd_yytext );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
      }

    } else if( tok == T_EOF ) {

      break;

    }

  }

  /* Simulate the last timestep now */
  if( use_last_timestep && simulate ) {
    (void)db_do_timestep( last_timestep, FALSE );
  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw vcd_parse_def vcd_parse_sim

 Reads specified VCD file for relevant information and calls the database
 functions when appropriate to store this information.  This replaces the
 need for a lexer and parser which should increase performance.
*/
void vcd_parse(
  const char* vcd_file  /*!< Name of VCD file to parse */
) { PROFILE(VCD_PARSE);

  FILE* vcd_handle;        /* Pointer to opened VCD file */

  if( (vcd_handle = fopen( vcd_file, "r" )) != NULL ) {

    unsigned int rv;

    /* Create initial symbol table */
    vcd_symtab = symtable_create();

    Try {

      /* Allocate memory for read buffer */
      vcd_rdbuf_start = vcd_rdbuf_end = vcd_rdbuf_cur = (char*)malloc_safe( VCD_BUFSIZE );

      /* Allocate memory for vcd_yytext */
      vcd_yytext = (char*)malloc_safe( (vcd_yytext_size = 1024) );

      vcd_parse_def( vcd_handle );

      /* Create timestep symbol table array */
      if( vcd_symtab_size > 0 ) {
        timestep_tab = malloc_safe_nolimit( sizeof( symtable*) * vcd_symtab_size );
      }
    
      vcd_parse_sim( vcd_handle );

    } Catch_anonymous {
      symtable_dealloc( vcd_symtab );
      free_safe( timestep_tab, (sizeof( symtable*) * vcd_symtab_size) );
      free_safe( vcd_rdbuf_start, VCD_BUFSIZE );
      free_safe( vcd_yytext, vcd_yytext_size );
      rv = fclose( vcd_handle );
      assert( rv == 0 );
      Throw 0;
    }

    /* Deallocate memory */
    symtable_dealloc( vcd_symtab );
    free_safe( timestep_tab, (sizeof( symtable*) * vcd_symtab_size) );
    free_safe( vcd_rdbuf_start, VCD_BUFSIZE );
    free_safe( vcd_yytext, vcd_yytext_size );

    /* Close VCD file */
    rv = fclose( vcd_handle );
    assert( rv == 0 );

  } else {

    print_output( "Unable to open specified VCD file", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

