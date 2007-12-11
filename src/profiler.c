/*
 Copyright (c) 2000-2007 Trevor Williams

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
 \file    profiler.c
 \author  Trevor Williams  (phase1geo@gmail.com)
 \date    12/10/2007
*/

#include <stdio.h>
#include <assert.h>

#include "profiler.h"


/*! Current profiling mode value */
bool profiling_mode = TRUE;

/*! Name of output profiling file */
char* profiling_output = NULL;


extern char user_msg[USER_MSG_LENGTH];


/*!
 \param value  New value to set profiling mode to

 Sets the current profiling mode to the given value.
*/
void profiler_set_mode( bool value ) {

  profiling_mode = value;

}

/*!
 \param fname  Name of output profiling file.

 Sets the profiling output file to the given value.
*/
void profiler_set_filename( const char* fname ) {

  /* Deallocate profiling output name, if one was already specified */
  free_safe( profiling_output );

  profiling_output = strdup_safe( fname );

}

/*!
 Deallocates all allocated memory for profiler.
*/
void profiler_dealloc() {

  int i;  /* Loop iterator */

  /* Deallocate profiling output name */
  free_safe( profiling_output );

  /* Iterate through the profiler array and deallocate all timer structures */
  for( i=0; i<NUM_PROFILES; i++ ) {
    free_safe( profiles[i].time_in );
  }

}

void profiler_display_calls( FILE* ofile ) {

  int i;  /* Loop iterator */

  for( i=0; i<NUM_PROFILES; i++ ) {
    fprintf( ofile, "  %40s  %d\n", profiles[i].func_name, profiles[i].calls );
  }

}

/*!
 Generates profiling report if the profiling mode is set to TRUE.
*/
void profiler_report() {

  FILE* ofile;  /* File stream pointer to output file */

  if( profiling_mode ) {

    assert( profiling_output != NULL );

    if( (ofile = fopen( profiling_output, "w" )) != NULL ) {

      profiler_display_calls( ofile );

      /* Close the output file */
      fclose( ofile );

    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "Unable to open profiling output file \"%s\" for writing", profiling_output );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );

    }

  }

  /* Delete memory associated with the profiler */
  profiler_dealloc();

}


/*
 $Log$
 Revision 1.1  2007/12/10 23:16:22  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

*/

