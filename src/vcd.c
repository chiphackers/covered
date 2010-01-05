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
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "vcd.h"
#include "db.h"
#include "util.h"
#include "symtable.h"


extern char       user_msg[USER_MSG_LENGTH];
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;


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
 Parses specified file until $end keyword is seen, ignoring all text inbetween.
*/
static void vcd_parse_def_ignore(
  FILE* vcd  /*!< File handle pointer to opened VCD file */
) { PROFILE(VCD_PARSE_DEF_IGNORE);

  bool end_seen = FALSE;  /* If set to true, $end keyword was seen */
  char token[256];        /* String value of current token */
  int  chars_read;        /* Number of characters scanned in */

  while( !end_seen && (fscanf( vcd, "%s%n", token, &chars_read ) == 1) ) {
    assert( chars_read <= 256 );
    if( strncmp( "$end", token, 4 ) == 0 ) {
      end_seen = TRUE;
    }
  }

  assert( end_seen );

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw Throw

 Parses definition $var keyword line until $end keyword is seen.
*/
static void vcd_parse_def_var(
  char* line  /*!< Pointer to line from VCD file */
) { PROFILE(VCD_PARSE_DEF_VAR);

  char type[256];     /* Variable type */
  int  size;          /* Bit width of specified variable */
  char id_code[256];  /* Unique variable identifier_code */
  char ref[256];      /* Name of variable in design */
  char reftmp[256];   /* Temporary variable name */
  char tmp[15];       /* Temporary string holder */
  int  msb = -1;      /* Most significant bit */
  int  lsb = -1;      /* Least significant bit */
  int  tmplsb;        /* Temporary LSB if swapping is needed */
  int  chars_read;

  if( sscanf( line, "%s %d %s %s %s%n", type, &size, id_code, ref, tmp, &chars_read ) == 5 ) {

    /* Make sure that we have not exceeded array boundaries */
    assert( strlen( type )    <= 256 );
    assert( strlen( ref )     <= 256 );
    assert( strlen( tmp )     <= 15  );
    assert( strlen( id_code ) <= 256 );

    line += chars_read;
    
    if( strncmp( "real", type, 4 ) == 0 ) {

      msb = 63;
      lsb = 0;

    } else {

      if( strncmp( "$end", tmp, 4 ) != 0 ) {

        /* A bit select was specified for this signal, get the size */
        if( sscanf( tmp, "\[%d:%d]", &msb, &lsb ) != 2 ) {
        
          if( sscanf( tmp, "\[%d]", &lsb ) != 1 ) {
            print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            msb = lsb;
          }

        }

        if( (sscanf( line, "%s", tmp ) != 1) || (strncmp( "$end", tmp, 4 ) != 0) ) {
          print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
          Throw 0;
        }

      } else if( sscanf( ref, "%[a-zA-Z0-9_]\[%s].", reftmp, tmp ) == 2 ) {

        /* This is a hierarchical reference so we shouldn't modify ref -- quirky behavior from VCS */
        msb = size - 1;
        lsb = 0;

      } else if( sscanf( ref, "%[a-zA-Z0-9_]\[%s]", reftmp, tmp ) == 2 ) {
  
        strcpy( ref, reftmp );
  
        if( sscanf( tmp, "%d:%d", &msb, &lsb ) != 2 ) {
          if( sscanf( tmp, "%d", &lsb ) != 1 ) {
            print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            msb = lsb;
          }
        }

      } else {

        msb = size - 1;
        lsb = 0;

      }

    }

  } else {

    print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
    Throw 0;
  
  }

  /* For now we will let any type and size slide */
  db_assign_symbol( ref, id_code, msb, lsb );
    
  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Parses definition $scope keyword line until $end keyword is seen.
*/
static void vcd_parse_def_scope(
  char* line  /*!< Line read from VCD file */
) { PROFILE(VCD_PARSE_DEF_SCOPE);

  char type[256];  /* Scope type */
  char id[256];    /* Name of scope to change to */

  if( sscanf( line, "%s %s $end", type, id ) == 2 ) {

    /* Make sure that we have not exceeded any array boundaries */
    assert( strlen( type ) <= 256 );
    assert( strlen( id )   <= 256 );
    
    /* For now we will let any type slide */
    db_set_vcd_scope( id );

  } else {

    print_output( "Unrecognized $scope format", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

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

  while( !enddef_found && util_readline( vcd, &line, &line_size ) ) {

    ptr = line;

    if( sscanf( ptr, "%s%n", keyword, &chars_read ) == 1 ) {

      assert( chars_read <= 256 );
    
      ptr = ptr + chars_read;

      if( keyword[0] == '$' ) {

        if( strncmp( "var", (keyword + 1), 3 ) == 0 ) {
          vcd_parse_def_var( ptr );
        } else if( strncmp( "scope", (keyword + 1), 5 ) == 0 ) {
          vcd_parse_def_scope( ptr );
        } else if( strncmp( "upscope", (keyword + 1), 7 ) == 0 ) {
          db_vcd_upscope();
        } else if( strncmp( "enddefinitions", (keyword + 1), 14 ) == 0 ) {
          enddef_found = TRUE;
        } else if( strncmp( "end", (keyword + 1), 3 ) == 0 ) {
          ignore = FALSE;
        } else {
          ignore = TRUE;
        }

      } else if( !ignore ) {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Non-keyword located where one should have been \"%s\"", keyword );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    }
  
  }

  /* Deallocate memory */
  free_safe( line, line_size );

  if( !enddef_found ) {
    print_output( "Specified VCD file is not a valid VCD file", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  assert( enddef_found );

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
  FILE* vcd,   /*!< File handle of opened VCD file */
  char* value  /*!< String containing value of current signal */
) { PROFILE(VCD_PARSE_SIM_VECTOR);

  char sym[256];    /* String value of signal symbol */
  int  chars_read;  /* Number of characters scanned in */

  if( fscanf( vcd, "%s%n", sym, &chars_read ) == 1 ) {

    assert( chars_read <= 256 );
    
    db_set_symbol_string( sym, value );

  } else {

    print_output( "Bad file format", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads the next token from the file and calls the appropriate database storage
 function for this signal change.
*/
static void vcd_parse_sim_real(
  FILE* vcd,   /*!< File handle of opened VCD file */
  char* value  /*!< String containing value of current signal */
) { PROFILE(VCD_PARSE_SIM_REAL);

  char sym[256];    /* String value of signal symbol */
  int  chars_read;  /* Number of characters scanned in */

  if( fscanf( vcd, "%s%n", sym, &chars_read ) == 1 ) {

    assert( chars_read <= 256 );

    db_set_symbol_string( sym, value );

  } else {

    print_output( "Bad file format", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads in symbol from simulation vector line that is to be ignored 
 (unused).  Signals an error message if the line is improperly formatted.
*/
/*@unused@*/ static void vcd_parse_sim_ignore(
  FILE* vcd  /*!< File handle of opened VCD file */
) { PROFILE(VCD_PARSE_SIM_IGNORE);

  char sym[256];    /* String value of signal symbol */
  int  chars_read;  /* Number of characters scanned in */

  if( fscanf( vcd, "%s%n", sym, &chars_read ) != 1 ) {

    print_output( "Bad file format", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }
  
  assert( chars_read <= 256 );

  PROFILE_END;

}

/*!
 \throws anonymous db_do_timestep db_do_timestep vcd_parse_sim_vector Throw vcd_parse_sim_ignore

 Parses all lines that occur in the simulation portion of the VCD file.
*/
static void vcd_parse_sim(
  FILE* vcd  /*!< File handle of opened VCD file */
) { PROFILE(VCD_PARSE_SIM);

  char   token[4100];                /* Current token from VCD file */
  uint64 last_timestep     = 0;      /* Value of last timestamp from file */
  bool   use_last_timestep = FALSE;  /* Specifies if timestep has been encountered */
  int    chars_read;                 /* Number of characters scanned in */
  bool   carry_over        = FALSE;  /* Specifies if last string was too long */
  bool   simulate          = TRUE;   /* Specifies if we should continue to simulate */
 
  while( !feof( vcd ) && (fscanf( vcd, "%4099s%n", token, &chars_read ) == 1) && simulate ) {

    if( chars_read < 4099 ) {
    
      if( token[0] == '$' ) {

        /* Ignore */

      } else if( (token[0] == 'b') || (token[0] == 'B') ) {

        vcd_parse_sim_vector( vcd, (token + 1) );

      } else if( (token[0] == 'r') || (token[0] == 'R') || carry_over ) {

        vcd_parse_sim_real( vcd, (token + 1) );
        carry_over = FALSE;

      } else if( token[0] == '#' ) {

        if( use_last_timestep ) {
          simulate = db_do_timestep( last_timestep, FALSE );
        }
        last_timestep = ato64( token + 1 );
        use_last_timestep = TRUE;

      } else if( (token[0] == '0') ||
                 (token[0] == '1') ||
                 (token[0] == 'x') ||
                 (token[0] == 'X') ||
                 (token[0] == 'z') ||
                 (token[0] == 'Z') ) {

        db_set_symbol_char( token + 1, token[0] );

      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Badly placed token \"%s\"", token );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    } else {
 
      carry_over = TRUE;

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

      vcd_parse_def( vcd_handle );

      /* Create timestep symbol table array */
      if( vcd_symtab_size > 0 ) {
        timestep_tab = malloc_safe_nolimit( sizeof( symtable*) * vcd_symtab_size );
      }
    
      vcd_parse_sim( vcd_handle );

    } Catch_anonymous {
      symtable_dealloc( vcd_symtab );
      free_safe( timestep_tab, (sizeof( symtable*) * vcd_symtab_size) );
      rv = fclose( vcd_handle );
      assert( rv == 0 );
      Throw 0;
    }

    /* Deallocate memory */
    symtable_dealloc( vcd_symtab );
    free_safe( timestep_tab, (sizeof( symtable*) * vcd_symtab_size) );

    /* Close VCD file */
    rv = fclose( vcd_handle );
    assert( rv == 0 );

  } else {

    print_output( "Unable to open specified VCD file", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

