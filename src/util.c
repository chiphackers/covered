/*!
 \file     util.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#ifdef HAVE_MPATROL
#include <mpdebug.h>
#endif

#include "defines.h"
#include "util.h"

/*!
 If set to TRUE, suppresses all non-fatal error messages from being displayed.
*/
bool output_suppressed;

/*!
 Contains the total number of bytes malloc'ed during the simulation run.  This
 information is output to the user after simulation as a performance indicator.
*/
unsigned long curr_malloc_size = 0;

/*!
 Holds the largest number of bytes in allocation at one period of time.
*/
unsigned long largest_malloc_size = 0;


/*!
 \param value Boolean value of suppression.

 Sets the global variable output_suppressed to the specified value.
*/
void set_output_suppression( bool value ) {

  output_suppressed = value;

}

/*!
 \param msg   Message to display.
 \param type  Type of message to output

 Displays the specified message to standard output based on the type of message
 being output.
*/
void print_output( char* msg, int type ) {

  switch( type ) {
    case NORMAL:
      if( !output_suppressed ) { printf( "%s\n", msg ); }
      break;
    case WARNING:
      if( !output_suppressed ) { fprintf( stderr, "Warning!  %s\n", msg ); }
      break;
    case FATAL:
      fprintf( stderr, "Error!  %s\n", msg );
      break;
    default:  break;
  }

}

/*!
 \param token String to check for valid variable name.
 \return Returns TRUE if the specified string is a legal variable name; otherwise,
         returns FALSE.

 If the specified string follows all of the rules for a legal program
 variable (doesn't start with a number, contains only a-zA-Z0-9_ characters),
 returns a value of TRUE; otherwise, returns a value of FALSE.
*/
bool is_variable( char* token ) {

  bool retval = TRUE;   /* Return value of this function */

  if( token != NULL ) {

    if( (token[0] >= '0') && (token[0] <= '9') ) {

      retval = FALSE;

    } else {

      while( (token[0] != '\0') && retval ) {
        if( !(((token[0] >= 'a') && (token[0] <= 'z')) ||
             ((token[0] >= 'A') && (token[0] <= 'Z')) ||
             ((token[0] >= '0') && (token[0] <= '9')) ||
              (token[0] == '_')) ) {
          retval = FALSE;
        }
        token++;
      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param token String to check for valid pathname-ness
 \return Returns TRUE if the specified string is a legal UNIX directory;
         otherwise, returns FALSE.

 Returns TRUE if the specified string is either a legal UNIX relative
 pathname or static pathname.  If the specified string does not correlate
 to a legal UNIX pathname, a value of FALSE is returned.
*/
bool is_directory( char* token ) {

  bool retval  = TRUE;    /* Return value of this function   */
  int  periods = 0;       /* Number of periods seen in a row */

  if( token != NULL ) {

    while( (token[0] != '\0') && retval ) {
      if( token[0] == '.' ) {
        periods++;
        if( periods > 2 ) {
          retval = FALSE;
        }
      } else if( token[0] == '/' ) {
        periods = 0;
      } else if( ((token[0] >= 'a') && (token[0] <= 'z')) ||
                 ((token[0] >= 'A') && (token[0] <= 'Z')) ||
                 ((token[0] >= '0') && (token[0] <= '9')) ||
                  (token[0] == '_') ) {
        periods = 0;
      } else {
        retval = FALSE;
      }
      token++;
    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param dir Name of directory to check for existence.
 \return Returns TRUE if the specified directory exists; otherwise, returns FALSE.

 Checks to see if the specified directory actually exists in the file structure.
 If the directory is found to exist, returns TRUE; otherwise, returns FALSE.
*/
bool directory_exists( char* dir ) {

  bool        retval = FALSE;  /* Return value for this function    */
  struct stat filestat;        /* Statistics of specified directory */

  if( stat( dir, &filestat ) == 0 ) {

    if( S_ISDIR( filestat.st_mode ) ) {

      retval = TRUE;

    }

  }

  return( retval );

}

/*!
 \param file Name of file to check for existence.
 \return Returns TRUE if the specified file exists; otherwise, returns FALSE.

 Checks to see if the specified file actually exists in the file structure.
 If the file is found to exist, returns TRUE; otherwise, returns FALSE.
*/
bool file_exists( char* file ) {

  bool        retval = FALSE;  /* Return value for this function    */
  struct stat filestat;        /* Statistics of specified directory */

  if( stat( file, &filestat ) == 0 ) {

    if( S_ISREG( filestat.st_mode ) ) {

      retval = TRUE;

    }

  }

  return( retval );

}

/*!
 \param file  File to read next line from.
 \param line  Pointer to string which will contain read line minus newline character.
 
 \return Returns FALSE if feof is encountered; otherwise, returns TRUE.

 Reads in a single line of information from the specified file and returns a string
 containing the read line to the calling function.
*/
bool readline( FILE* file, char** line ) {

  char  c;                 /* Character recently read from file */
  int   i         = 0;     /* Current index of line             */
  int   line_size = 128;   /* Size of current line              */

  *line = (char*)malloc_safe( line_size );

  while( !feof( file ) && ((c = (char)fgetc( file )) != '\n') ) {

    if( i == (line_size - 1) ) {
      line_size = line_size * 2;
      *line     = (char*)realloc( *line, line_size );
    }

    (*line)[i] = c;
    i++;

  }

  if( !feof( file ) ) {
    (*line)[i] = '\0';
  } else {
    free_safe( *line );
    *line = NULL;
  }

  return( !feof( file ) );

}

/*!
 \param scope   Full scope to extract from.
 \param front   Highest level of hierarchy extracted.
 \param rest    Hierarchy left after extraction.
 
 Extracts the highest level of hierarchy from the specified scope,
 returning that instance name to the value of front and the the
 rest of the hierarchy in the value of rest.
*/
void scope_extract_front( char* scope, char* front, char* rest ) {
  
  char* ptr;   /* Pointer to current character */
  
  ptr = scope;
  
  while( (*ptr != '\0') && (*ptr != '.') ) {
    ptr++;
  }
  
  strncpy( front, scope, (ptr - scope) );
  front[ (ptr - scope) ] = '\0';
  
  if( *ptr == '.' ) {
    ptr++;
    strncpy( rest, ptr, (strlen( scope ) - (ptr - scope) - 1) );
    rest[ (strlen( scope ) - (ptr - scope) - 1) ] = '\0';
  } else {
    rest[0] = '\0';
  }
  
}

/*!
 \param scope  Full scope to extract from.
 \param back   Lowest level of hierarchy extracted.
 \param rest   Hierarchy left after extraction.
 
 Extracts the lowest level of hierarchy from the specified scope,
 returning that instance name to the value of back and the the
 rest of the hierarchy in the value of rest.
*/
void scope_extract_back( char* scope, char* back, char* rest ) {

  char* ptr;    /* Pointer to current character */

  ptr = scope + strlen( scope ) - 1;

  while( (ptr > scope) && (*ptr != '.') ) {
    ptr--;
  }

  strncpy( rest, scope, (ptr - scope) );
  rest[ (ptr - scope) ] = '\0';

  if( *ptr == '.' ) {
    ptr++;
  }

  strncpy( back, ptr, ((strlen( scope ) + scope) - ptr) );
  back[ ((strlen( scope ) + scope) - ptr) ] = '\0';
  
}

/*!
 \param size  Number of bytes to allocate.

 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.
*/
void* malloc_safe( int size ) {

  void* obj;      /* Object getting malloc address */

  if( size > 10000 ) {
    print_output( "Allocating memory chunk larger than 10000 bytes.  Possible error.", WARNING );
    // printf( "  Memory block size request: %d bytes\n", size );
    assert( size > 10000 );
  } else if( size <= 0 ) {
    print_output( "Internal:  Attempting to allocate memory of size <= 0", FATAL );
    assert( size <= 0 );
  }

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  // printf( "Malloc size: %d, curr: %ld, largest: %ld\n", size, curr_malloc_size, largest_malloc_size );

  obj = malloc( size );

  if( obj == NULL ) {
    print_output( "Out of heap memory", FATAL );
    exit( 1 );
  }

  return( obj );

}

/*!
 \param ptr  Pointer to object to deallocate.

 Safely performs a free function of heap memory.  Also keeps track
 of current memory usage for output information at end of program
 life.
*/
void free_safe( void* ptr ) {

  curr_malloc_size -= sizeof( *ptr );

  free( ptr );

}

/*!
 \param spaces  Pointer to string to places spaces into.
 \param num_spaces  Number of spaces to place in string.
 
 Creates a string that contains num_spaces number of space characters,
 adding a NULL character at the end of the string to allow for correct
 usage by the strlen and other string functions.
*/
void gen_space( char* spaces, int num_spaces ) {

  int i;     /* Loop iterator */

  for( i=0; i<num_spaces; i++ ) {
    spaces[i] = ' ';
  }

  spaces[i] = '\0';
  
}
  
