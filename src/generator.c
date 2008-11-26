/*
 Copyright (c) 2006-2008 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "defines.h"
#include "func_iter.h"
#include "generator.h"
#include "profiler.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern char         user_msg[USER_MSG_LENGTH];


/*!
 Generates an instrumented version of a given functional unit.
*/
static void generator_output_funit(
  func_unit* funit  /*!< Pointer to functional unit to output */
) { PROFILE(GENERATOR_OUTPUT_FUNIT);

  printf( "funit %s, type: %s\n", funit->name, get_funit_type( funit->type ) );

  /* Only generate output for a given module (and it is not $root) */
  if( (funit->type == FUNIT_MODULE) && (strncmp( "$root", funit->name, 5 ) != 0) ) {

    FILE*        ofile;
    char         filename[4096];
    unsigned int rv;

    printf( "HERE A!\n" );

    /* TBD - The extension should probably be settable by the user */
    rv = snprintf( filename, 4096, "covered/verilog/%s.v", funit->name );
    assert( rv < 4096 );

    printf( "Going to create %s\n", filename );

    /* Open the output file for writing */
    if( (ofile = fopen( filename, "w" )) != NULL ) {

      FILE* ifile;

      printf( "module filename: %s\n", funit->filename );

      /* Open the original file for reading */
      if( (ifile = fopen( funit->filename, "r" )) != NULL ) {

        func_iter    fi;
        char*        line;
        unsigned int line_size;
        unsigned int line_num = 1;

        /* Initialize the functional unit iterator */
        func_iter_init( &fi, funit, TRUE, TRUE );

        /* Read in each line */
        while( util_readline( ifile, &line, &line_size ) ) {
          if( (line_num >= funit->start_line) && (line_num <= funit->end_line) ) {
            fprintf( ofile, "%s\n", line );
          }
          free_safe( line, line_size );
          line_num++;
        }

        /* Deallocate functional unit iterator */
        func_iter_dealloc( &fi );

        /* Close the input file */
        rv = fclose( ifile );
        assert( rv == 0 );

      } else {

        rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open \"%s\" for reading", funit->filename );
        assert( rv < USER_MSG_LENGTH );
        fclose( ofile );
        Throw 0;

      }

      /* Close output file */
      rv = fclose( ofile );
      assert( rv == 0 );

    } else {

      rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to create generated Verilog file \"%s\"", filename );
      assert( rv < USER_MSG_LENGTH );
      Throw 0;

    }

  }

  PROFILE_END;

}

/*!
 Outputs the covered portion of the design to the covered/verilog directory.
*/
void generator_output() { PROFILE(GENERATOR_OUTPUT);

  funit_link* funitl = db_list[curr_db]->funit_head;

  /* Create the initial "covered" directory - TBD - this should be done prior to this function call */
  if( !directory_exists( "covered" ) ) {
    if( mkdir( "covered", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
      print_output( "Unable to create \"covered\" directory", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* If the "covered/verilog" directory exists, remove it */
  if( directory_exists( "covered/verilog" ) ) {
    if( system( "rm -rf covered/verilog" ) != 0 ) {
      print_output( "Unable to remove \"covered/verilog\" directory", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* Create "covered/verilog" directory */
  if( mkdir( "covered/verilog", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
    print_output( "Unable to create \"covered/verilog\" directory", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Iterate through the functional units, outputting the modules to their own files in the covered/verilog directory */
  while( funitl != NULL ) {
    generator_output_funit( funitl->funit );
    funitl = funitl->next;
  }

  PROFILE_END;

}

/*
 $Log$
 Revision 1.1  2008/11/25 23:53:07  phase1geo
 Adding files for Verilog generator functions.

*/

