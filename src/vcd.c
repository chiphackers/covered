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
extern char*      top_instance;
extern bool       instance_specified;
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;


/*!
 This flag is used to indicate if Covered was successfull in finding at least one
 matching instance from the VCD file.  If no instances were found for the entire
 VCD file, the user has either not specified the -i option or has specified an
 incorrect path to the top-level module instance so let them know about this.
*/
bool one_instance_found = FALSE;


/*!
 \param vcd  File handle pointer to opened VCD file.

 Parses specified file until $end keyword is seen, ignoring all text inbetween.
*/
static void vcd_parse_def_ignore(
  FILE* vcd
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
 \param vcd  File handle pointer to opened VCD file.

 \throws anonymous Throw Throw Throw Throw

 Parses definition $var keyword line until $end keyword is seen.
*/
static void vcd_parse_def_var(
  FILE* vcd
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

  if( fscanf( vcd, "%s %d %s %s %s", type, &size, id_code, ref, tmp ) == 5 ) {

    /* Make sure that we have not exceeded array boundaries */
    assert( strlen( type )    <= 256 );
    assert( strlen( ref )     <= 256 );
    assert( strlen( tmp )     <= 15  );
    assert( strlen( id_code ) <= 256 );
    
    if( strncmp( "$end", tmp, 4 ) != 0 ) {

      /* A bit select was specified for this signal, get the size */
      if( sscanf( tmp, "\[%d:%d]", &msb, &lsb ) != 2 ) {
        
        if( sscanf( tmp, "\[%d]", &lsb ) != 1 ) {
          print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
          printf( "vcd Throw A\n" );
          Throw 0;
        } else {
          msb = lsb;
        }

      }

      if( (fscanf( vcd, "%s", tmp ) != 1) || (strncmp( "$end", tmp, 4 ) != 0) ) {
        print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
        printf( "vcd Throw B\n" );
        Throw 0;
      }

    } else if( sscanf( ref, "%[a-zA-Z0-9_]\[%s]", reftmp, tmp ) == 2 ) {

      strcpy( ref, reftmp );

      if( sscanf( tmp, "%d:%d", &msb, &lsb ) != 2 ) {
        if( sscanf( tmp, "%d", &lsb ) != 1 ) {
          print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
          printf( "vcd Throw C\n" );
          Throw 0;
        } else {
          msb = lsb;
        }
      }

    } else {

      msb = size - 1;
      lsb = 0;

    }

    /* If the signal is output in big endian format, swap the lsb and msb values accordingly */
    if( lsb > msb ) {
      tmplsb = lsb;
      lsb    = msb;
      msb    = tmplsb;
    }

    /* For now we will let any type and size slide */
    db_assign_symbol( ref, id_code, msb, lsb );
    
  } else {

    print_output( "Unrecognized $var format", FATAL, __FILE__, __LINE__ );
    printf( "vcd Throw D\n" );
    Throw 0;
  
  }

  PROFILE_END;

}

/*!
 \param vcd  File handle pointer to opened VCD file.

 \throws anonymous Throw

 Parses definition $scope keyword line until $end keyword is seen.
*/
static void vcd_parse_def_scope(
  FILE* vcd
) { PROFILE(VCD_PARSE_DEF_SCOPE);

  char type[256];  /* Scope type */
  char id[256];    /* Name of scope to change to */

  if( fscanf( vcd, "%s %s $end", type, id ) == 2 ) {

    /* Make sure that we have not exceeded any array boundaries */
    assert( strlen( type ) <= 256 );
    assert( strlen( id )   <= 256 );
    
    /* For now we will let any type slide */
    db_set_vcd_scope( id );

  } else {

    print_output( "Unrecognized $scope format", FATAL, __FILE__, __LINE__ );
    printf( "vcd Throw E\n" );
    Throw 0;

  }

  PROFILE_END;
  
}

/*!
 \param vcd  File handle pointer to opened VCD file.

 \throws anonymous Throw Throw Throw vcd_parse_def_scope vcd_parse_def_var

 Parses all definition information from specified file.
*/
static void vcd_parse_def(
  FILE* vcd
) { PROFILE(VCD_PARSE_DEF);

  bool enddef_found = FALSE;  /* If set to true, definition section is finished */
  char keyword[256];          /* Holds keyword value */
  int  chars_read;            /* Number of characters scanned in */

  while( !enddef_found && (fscanf( vcd, "%s%n", keyword, &chars_read ) == 1) ) {

    assert( chars_read <= 256 );
    
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

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Non-keyword located where one should have been \"%s\"", keyword );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      printf( "vcd Throw F\n" );
      Throw 0;

    }
  
  }

  if( !enddef_found ) {
    print_output( "Specified VCD file is not a valid VCD file", FATAL, __FILE__, __LINE__ );
    printf( "vcd Throw G\n" );
    Throw 0;
  }

  assert( enddef_found );

  /* Check to see that at least one instance was found */
  if( !one_instance_found ) {

    print_output( "No instances were found in specified VCD file that matched design", FATAL, __FILE__, __LINE__ );

    /* If the -i option was not specified, let the user know */
    if( !instance_specified ) {
      print_output( "  Please use -i option to specify correct hierarchy to top-level module to score",
                    FATAL, __FILE__, __LINE__ );
    } else {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "  Incorrect hierarchical path specified in -i option: %s", top_instance );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
    }

    printf( "vcd Throw H\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param vcd    File handle of opened VCD file.
 \param value  String containing value of current signal.

 \throws anonymous Throw

 Reads the next token from the file and calls the appropriate database storage
 function for this signal change.
*/
static void vcd_parse_sim_vector(
  FILE* vcd,
  char* value
) { PROFILE(VCD_PARSE_SIM_VECTOR);

  char sym[256];    /* String value of signal symbol */
  int  chars_read;  /* Number of characters scanned in */

  if( fscanf( vcd, "%s%n", sym, &chars_read ) == 1 ) {

    assert( chars_read <= 256 );
    
    db_set_symbol_string( sym, value );

  } else {

    print_output( "Bad file format", FATAL, __FILE__, __LINE__ );
    printf( "vcd Throw I\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param vcd  File handle of opened VCD file.

 \throws anonymous Throw

 Reads in symbol from simulation vector line that is to be ignored 
 (unused).  Signals an error message if the line is improperly formatted.
*/
static void vcd_parse_sim_ignore(
  FILE* vcd
) { PROFILE(VCD_PARSE_SIM_IGNORE);

  char sym[256];    /* String value of signal symbol */
  int  chars_read;  /* Number of characters scanned in */

  if( fscanf( vcd, "%s%n", sym, &chars_read ) != 1 ) {

    print_output( "Bad file format", FATAL, __FILE__, __LINE__ );
    printf( "vcd Throw J\n" );
    Throw 0;

  }
  
  assert( chars_read <= 256 );

  PROFILE_END;

}

/*!
 \param vcd  File handle of opened VCD file.

 \throws anonymous db_do_timestep db_do_timestep vcd_parse_sim_vector Throw vcd_parse_sim_ignore

 Parses all lines that occur in the simulation portion of the VCD file.
*/
static void vcd_parse_sim(
  FILE* vcd
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

        vcd_parse_sim_ignore( vcd );
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
        printf( "vcd Throw K\n" );
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
 \param vcd_file  Name of VCD file to parse.

 \throws anonymous Throw Throw vcd_parse_def vcd_parse_sim

 Reads specified VCD file for relevant information and calls the database
 functions when appropriate to store this information.  This replaces the
 need for a lexer and parser which should increase performance.
*/
void vcd_parse(
  const char* vcd_file
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
      free_safe( timestep_tab );
      rv = fclose( vcd_handle );
      assert( rv == 0 );
      printf( "vcd Throw L\n" );
      Throw 0;
    }

    /* Deallocate memory */
    symtable_dealloc( vcd_symtab );
    free_safe( timestep_tab );

    /* Close VCD file */
    rv = fclose( vcd_handle );
    assert( rv == 0 );

  } else {

    print_output( "Unable to open specified VCD file", FATAL, __FILE__, __LINE__ );
    printf( "vcd Throw M\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*
 $Log$
 Revision 1.39  2008/03/13 10:28:55  phase1geo
 The last of the exception handling modifications.

 Revision 1.38  2008/03/11 22:06:49  phase1geo
 Finishing first round of exception handling code.

 Revision 1.37  2008/02/29 23:58:19  phase1geo
 Continuing to work on adding exception handling code.

 Revision 1.36  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.35  2008/01/29 04:40:19  phase1geo
 Fixing bug with VPI builds on Mac OS X.  Removing attempt to get Veriwell simulator
 to work with VCD reader.  Updates to regression Makefiles.

 Revision 1.34  2008/01/28 15:05:08  phase1geo
 Attempt to fix issue with Veriwell regressions (still some work to do here).

 Revision 1.33  2008/01/15 23:01:15  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.32  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.31  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.30  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.29  2007/12/11 23:19:14  phase1geo
 Fixed compile issues and completed first pass injection of profiling calls.
 Working on ordering the calls from most to least.

 Revision 1.28  2007/11/20 05:31:14  phase1geo
 Fixing syntax error in VCD parser per bug 1832592.

 Revision 1.27  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.26  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.25  2006/11/03 19:00:01  phase1geo
 Fixing bug 1589831.  Also added err3 and err3.1 diagnostics to verify its
 correct behavior.

 Revision 1.24  2006/08/30 12:02:48  phase1geo
 Changing assertion in vcd.c that fails when the VCD file is improperly formatted
 to a user error message with a bit more meaning.  Fixing problem with signedness
 of enumeration resolution.  Added enum1.1 diagnostic to testsuite.

 Revision 1.23  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.22  2006/05/25 12:11:02  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.21  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.20.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.20  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.19  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

 Revision 1.18  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.17  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.16  2003/11/11 22:41:54  phase1geo
 Fixing bug with reading VCD signals that are too long for Covered to support.
 We were getting an assertion error when we should have simply read in and ignored
 the signal.

 Revision 1.15  2003/10/19 04:23:49  phase1geo
 Fixing bug in VCD parser for new Icarus Verilog VCD dumpfile formatting.
 Fixing bug in signal.c for signal merging.  Updates all CDD files to match
 new format.  Added new diagnostics to test advanced FSM state variable
 features.

 Revision 1.14  2003/10/07 03:10:04  phase1geo
 Fixing VCD reader to allow vector selects that are attached (no spaces) to
 reference names to be read properly.  New version of Icarus seems to output
 in this way now.  Full regression passes.

 Revision 1.13  2003/08/21 21:57:30  phase1geo
 Fixing bug with certain flavors of VCD files that alias signals that have differing
 MSBs and LSBs.  This takes care of the rest of the bugs for the 0.2 stable release.

 Revision 1.12  2003/08/20 22:08:39  phase1geo
 Fixing problem with not closing VCD file after VCD parsing is completed.
 Also fixed memory problem with symtable.c to cause timestep_tab entries
 to only be loaded if they have not already been loaded during this timestep.
 Also added info.h to include list of db.c.

 Revision 1.11  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.10  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.9  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.8  2003/02/07 23:12:30  phase1geo
 Optimizing db_add_statement function to avoid memory errors.  Adding check
 for -i option to avoid user error.

 Revision 1.7  2003/01/03 05:52:00  phase1geo
 Adding code to help safeguard from segmentation faults due to array overflow
 in VCD parser and symtable.  Reorganized code for symtable symbol lookup and
 value assignment.

 Revision 1.6  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.5  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.4  2002/10/12 22:21:35  phase1geo
 Making code fix for parameters when parameter is used in calculation of
 signal size.  Also adding parse ability for real numbers in a VCD file
 (though real number support is still avoided).

 Revision 1.3  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.2  2002/09/18 22:19:25  phase1geo
 Adding handler for option bit select in $var line.

 Revision 1.1  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.
*/

