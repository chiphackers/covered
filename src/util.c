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
 \file     util.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
#include <tcl.h>
#include <tk.h> 
#endif
#endif

#ifdef HAVE_MPATROL
#include <mpdebug.h>
#endif

#include "defines.h"
#include "util.h"
#include "link.h"

extern bool        report_gui;
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
extern Tcl_Interp* interp;
#endif
#endif

/*!
 If set to TRUE, suppresses all non-fatal error messages from being displayed.
*/
bool output_suppressed;

/*!
 If set to TRUE, causes debug information to be spewed to screen.
*/
bool debug_mode;

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
 Holds some output that will be displayed via the print_output command.  This is
 created globally so that memory does not need to be reallocated for each function
 that wishes to use it.
*/
char user_msg[USER_MSG_LENGTH];

/*!
 Array of functional unit names used for output purposes.
*/
const char* funit_types[FUNIT_TYPES+1] = { "module", "named block", "function", "task", "UNKNOWN" };


/*!
 \param value Boolean value of suppression.

 Sets the global variable output_suppressed to the specified value.
*/
void set_output_suppression( bool value ) {

  output_suppressed = value;

}

/*!
 \param value  Boolean value of debug mode.

 Sets the global debug mode to the specified value.
*/
void set_debug( bool value ) {

  debug_mode = value;

}

/*!
 \param msg   Message to display.
 \param type  Type of message to output
 \param file  Name of file that called this function
 \param line  Line number that this function was called in

 Displays the specified message to standard output based on the type of message
 being output.
*/
void print_output( char* msg, int type, char* file, int line ) {

  FILE* outf = debug_mode ? stdout : stderr;
  char  tmpmsg[USER_MSG_LENGTH];

  switch( type ) {
    case DEBUG:
      if( debug_mode ) {
#ifdef VPI_ONLY
        vpi_printf( "%s\n", msg );
#else
        printf( "%s\n", msg );
#endif
      }
      break;
    case NORMAL:
      if( !output_suppressed || debug_mode ) {
#ifdef VPI_ONLY
        vpi_printf( "%s\n", msg );
#else
        printf( "%s\n", msg );
#endif
      }
      break;
    case WARNING:
      if( !output_suppressed ) {
        if( report_gui ) {
          snprintf( tmpmsg, USER_MSG_LENGTH, "WARNING!  %s\n", msg );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
#endif
#endif
        } else {
          fprintf( outf, "    WARNING!  %s\n", msg );
        }
      } else if( debug_mode ) {
        if( report_gui ) {
          snprintf( tmpmsg, USER_MSG_LENGTH, "WARNING!  %s (file: %s, line: %d)\n", msg, file, line );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
#endif
#endif
        } else {
          fprintf( outf, "    WARNING!  %s (file: %s, line: %d)\n", msg, file, line );
        }
      }
      break;
    case WARNING_WRAP:
      if( !output_suppressed || debug_mode ) {
        if( report_gui ) {
          snprintf( tmpmsg, USER_MSG_LENGTH, "              %s\n", msg );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_AppendElement( interp, tmpmsg );
#endif
#endif
        } else {
          fprintf( outf, "              %s\n", msg );
        }
      }
      break; 
    case FATAL:
      if( debug_mode ) {
        if( report_gui ) {
          snprintf( tmpmsg, USER_MSG_LENGTH, "%s (file: %s, line: %d)\n", msg, file, line );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
#endif
#endif
        } else {
          fprintf( stderr, "ERROR!  %s (file: %s, line: %d)\n", msg, file, line );
        }
      } else {
        if( report_gui ) {
          snprintf( tmpmsg, USER_MSG_LENGTH, "%s\n", msg );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
#endif
#endif
        } else {
          fprintf( stderr, "ERROR!  %s\n", msg );
        }
      }
      break;
    case FATAL_WRAP:
      if( report_gui ) {
        snprintf( tmpmsg, USER_MSG_LENGTH, "%s\n", msg );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
        Tcl_AppendElement( interp, tmpmsg );
#endif
#endif
      } else { 
        fprintf( stderr, "        %s\n", msg );
      }
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
 \param token  Pointer to string to parse.

 \return Returns TRUE if the specified token is a valid argument representing
         a functional unit.
*/
bool is_func_unit( char* token ) {

  char* orig;                          /* Temporary string */
  char* rest;                          /* Temporary string */
  char* front;                         /* Temporary string */
  bool  okay = (strlen( token ) > 0);  /* Specifies if this token is a functional unit value or not */

  /* Allocate memory */
  orig  = strdup_safe( token, __FILE__, __LINE__ );
  rest  = strdup_safe( token, __FILE__, __LINE__ );
  front = strdup_safe( token, __FILE__, __LINE__ );

  /* Check to make sure that each value between '.' is a valid variable */
  while( (strlen( orig ) > 0) && okay ) {
    scope_extract_front( orig, front, rest );
    if( !is_variable( front ) ) {
      okay = FALSE;
    } else {
      strcpy( orig, rest );
    }
  }

  /* Deallocate memory */
  free_safe( orig );
  free_safe( rest );
  free_safe( front );

  return( okay );

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
 \param str  String containing pathname to file.

 \return Returns pointer to string containing only base filename.

 Extracts the file basename of the specified filename string.
*/
char* get_basename( char* str ) {

  char* ptr;  /* Pointer to current character in str */

  ptr = (str + strlen( str )) - 1;

  while( (ptr > str) && (*ptr != '/') ) {
    ptr--;
  }

  if( *ptr == '/' ) {
    ptr++;
  }

  return( ptr );

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
 \param dir        Name of directory to read files from.
 \param ext_head   Pointer to extension list.
 \param file_head  Pointer to head element of filename string list.
 \param file_tail  Pointer to tail element of filename string list.

 \bug Need to order files according to extension first instead of filename.

 Opens the specified directory for reading and loads (in order) all files that
 contain the specified extensions (if ext_head is NULL, load only *.v files).
 Stores all string filenames to the specified string list.
*/
void directory_load( char* dir, str_link* ext_head, str_link** file_head, str_link** file_tail ) {

  DIR*           dir_handle;  /* Pointer to opened directory                                 */
  struct dirent* dirp;        /* Pointer to current directory entry                          */
  str_link*      curr_ext;    /* Pointer to current extension string                         */
  char*          ptr;         /* Pointer to current character in filename                    */
  int            tmpchars;    /* Number of characters needed to store full pathname for file */
  char*          tmpfile;     /* Temporary string holder for full pathname of file           */

  if( (dir_handle = opendir( dir )) == NULL ) {

    snprintf( user_msg, USER_MSG_LENGTH, "Unable to read directory %s", dir );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );

  } else {

    while( (dirp = readdir( dir_handle )) != NULL ) {
      ptr = dirp->d_name + strlen( dirp->d_name ) - 1;
      /* Work backwards until a dot is encountered */
      while( (ptr >= dirp->d_name) && (*ptr != '.') ) {
        ptr--;
      }
      if( *ptr == '.' ) {
        ptr++;
        curr_ext = ext_head;
        while( (curr_ext != NULL) && (strcmp( ptr, curr_ext->str ) != 0) ) {
          curr_ext = curr_ext->next;
        }
        if( curr_ext != NULL ) {
          /* Found valid extension, add to list */
          tmpchars = strlen( dirp->d_name ) + strlen( dir ) + 2;
          tmpfile  = (char*)malloc_safe( tmpchars, __FILE__, __LINE__ );
          snprintf( tmpfile, tmpchars, "%s/%s", dir, dirp->d_name );
          if( str_link_find( tmpfile, *file_head ) == NULL ) {
            str_link_add( tmpfile, file_head, file_tail );
            (*file_tail)->suppl = 0x1;
          }
        }
      }
    }

    closedir( dir_handle );

  }

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
bool util_readline( FILE* file, char** line ) {

  char  c;                 /* Character recently read from file */
  int   i         = 0;     /* Current index of line             */
  int   line_size = 128;   /* Size of current line              */

  *line = (char*)malloc_safe( line_size, __FILE__, __LINE__ );

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
  
  char* ptr;      /* Pointer to current character */
  char  endchar;  /* Set to the character we are searching for */
  
  ptr = scope;

  /* Figure out if we are looking for a '.' or a ' ' character */
  endchar = (*ptr == '\\') ? ' ' : '.';
  
  while( (*ptr != '\0') && (*ptr != endchar) ) {
    ptr++;
  }

  /* If this is a literal, keep going until we see the '.' character */
  if( endchar == ' ' ) {
    while( (*ptr != '\0') && (*ptr != '.') ) {
      ptr++;
    }
  }
  
  strncpy( front, scope, (ptr - scope) );
  front[ (ptr - scope) ] = '\0';
  
  if( *ptr == '.' ) {
    ptr++;
    strncpy( rest, ptr, (strlen( scope ) - (ptr - scope)) );
    rest[ (strlen( scope ) - (ptr - scope)) ] = '\0';
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

  char* ptr;      /* Pointer to current character */
  char  endchar;  /* Set to the character we are searching for */

  ptr = scope + strlen( scope ) - 1;

  /* Figure out if we are looking for a '.' or a ' ' character */
  endchar = (*ptr == ' ') ? '\\' : '.';

  while( (ptr > scope) && (*ptr != endchar) ) {
    ptr--;
  }

  /* If this is a literal, keep going until we see the '.' character */
  if( endchar == '\\' ) {
    while( (ptr > scope) && (*ptr != '.') ) {
      ptr--;
    }
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
 \param scope
 \param front
 \param back

*/
void scope_extract_scope( char* scope, char* front, char* back ) {

  back[0] = '\0';

  if( (strncmp( scope, front, strlen( front ) ) == 0) && (strlen( scope ) > strlen( front )) ) {
    strcpy( back, (scope + strlen( front ) + 1) );
  }

}

/*!
 \param str  String to create printable version of

 \return Returns printable version of the given string (with any escaped sequences removed)

 Allocates memory for and generates a printable version of the given string (a signal or
 instance name).  The calling function is responsible for deallocating the string returned.
*/
char* scope_gen_printable( char* str ) {

  char* new_str;  /* New version of string with escaped sequences removed */

  /* Allocate memory for new string */
  new_str = strdup_safe( str, __FILE__, __LINE__ );

  /* Remove escape sequences, if any */
  if( str[0] == '\\' ) {
    sscanf( str, "\\%[^ \n\t\r\b]", new_str );
  }

  return( new_str );

} 

/*!
 \param str1  Pointer to signal/instance name
 \param str2  Pointer to signal/instance name

 \return Returns TRUE if the two strings are equal, properly handling the case where one or
         both are escaped names (start with an escape character and end with a space).
*/
bool scope_compare( char* str1, char* str2 ) {

  bool  retval;    /* Return value for this function */
  char* new_str1;  /* New form of str1 with escaped sequences removed */
  char* new_str2;  /* New form of str1 with escaped sequences removed */

  /* Create printable versions of the strings */
  new_str1 = scope_gen_printable( str1 );
  new_str2 = scope_gen_printable( str2 );

  /* Perform the compare */
  retval = (strcmp( new_str1, new_str2 ) == 0);

  /* Deallocate the memory */
  free_safe( new_str1 );
  free_safe( new_str2 );

  return( retval );

}

/*
 \param scope  Scope of some signal.

 \return Returns TRUE if specified scope is local to this module (no hierarchy given);
         otherwise, returns FALSE.

 Parses specified scope for '.' character.  If one is found, returns FALSE; otherwise,
 returns TRUE.
*/
bool scope_local( char* scope ) {

  char* ptr;             /* Pointer to current character */
  bool  esc;             /* Set to TRUE if current is escaped */
  bool  wspace = FALSE;  /* Set if last character seen was a whitespace */

  assert( scope != NULL );

  ptr = scope;
  esc = (*ptr == '\\');
  while( (*ptr != '\0') && ((*ptr != '.') || esc) ) {
    if( (*ptr == ' ') || (*ptr == '\n') || (*ptr == '\t') || (*ptr == '\b') || (*ptr == '\r') ) {
      esc    = FALSE;
      wspace = TRUE;
    } else {
      if( wspace && (*ptr == '\\') ) {
        esc = TRUE;
      }
    }
    ptr++;
  }

  return( *ptr == '\0' );

}

/*!
 \param mname  Name of module extracted.
 \param len    Length of mname string (we cannot exceed this value).
 \param fname  Name of filename to extract module name from.

 Takes in a filename (with possible directory information and/or possible extension)
 and transforms it into a filename with the directory and extension information stripped
 off.  Much like the the functionality of the unix command "basename".  Returns the 
 stripped filename in the mname parameter.
*/
void convert_file_to_module( char* mname, int len, char* fname ) {

  char* ptr;   /* Pointer to current character in filename */
  char* lptr;  /* Pointer to last character in module name */
  int   i;     /* Loop iterator                            */

  /* Set ptr to end of fname string */
  ptr  = fname + strlen( fname );
  lptr = ptr;

  /* Continue back until period is found */
  while( (ptr > fname) && (*ptr != '.') ) {
    ptr--;
  }

  if( ptr > fname ) {
    lptr = ptr;
  }

  /* Continue on until ptr == fname or we have reached a non-filename character */
  while( (ptr > fname) && (*ptr != '/') ) {
    ptr--;
  }

  /* Construct new name */
  if( ptr > fname ) {
    ptr++;
  }

  assert( (lptr - ptr) < len );

  i = 0;
  while( ptr < lptr ) {
    mname[i] = *ptr;
    ptr++;
    i++;
  }
  mname[i] = '\0';

}

  

/*!
 \param curr  Pointer to current file in list.
 \param mod   Name of module searching for.

 \return Returns pointer to next Verilog file to parse or NULL if no files were found.

 Iterates through specified file list, searching for next Verilog file to parse.
 If a file is a library file (suppl field is 'D'), the name of the module to search
 for is compared with the name of the file.
*/
str_link* get_next_vfile( str_link* curr, char* mod ) {

  str_link* next = NULL;  /* Pointer to next Verilog file to parse */
  char      name[256];    /* String holder for module name of file */

  while( (curr != NULL) && (next == NULL) ) {
    if( (curr->suppl & 0x1) != 0x1 ) {
      next = curr;
    } else {
      convert_file_to_module( name, 256, curr->str );
      if( strcmp( name, mod ) == 0 ) {
        next = curr;
      } else {
        curr = curr->next;
      }
    }
  }

  return( next );

}

/*!
 \param size  Number of bytes to allocate.
 \param file  File that called this function.
 \param line  Line number of file that called this function.

 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.
*/
void* malloc_safe( size_t size, char* file, int line ) {

  void* obj;      /* Object getting malloc address */

  if( size > 100000 ) {
    print_output( "Allocating memory chunk larger than 100000 bytes.  Possible error.", WARNING, file, line );
    assert( size <= 100000 );
  } else if( size <= 0 ) {
    print_output( "Internal:  Attempting to allocate memory of size <= 0", FATAL, file, line );
    assert( size > 0 );
  }

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = malloc( size );

  if( obj == NULL ) {
    print_output( "Out of heap memory", FATAL, file, line );
    exit( 1 );
  }

  return( obj );

}

/*!
 \param size  Number of bytes to allocate.
 \param file  Name of file that called this function.
 \param line  Line number of file that called this function.

 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.  Unlike
 malloc_safe, there is no upper bound on the amount of memory to allocate.
*/
void* malloc_safe_nolimit( size_t size, char* file, int line ) {

  void* obj;  /* Object getting malloc address */

  if( size <= 0 ) {
    print_output( "Internal:  Attempting to allocate memory of size <= 0", FATAL, file, line );
    assert( size > 0 );
  }

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = malloc( size );

  if( obj == NULL ) {
    print_output( "Out of heap memory", FATAL, file, line );
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

  if( ptr != NULL ) {
    free( ptr );
  }

}

/*!
 \param str   String to duplicate.
 \param file  Name of file that called this function.
 \param line  Line number of file that called this function.

 Calls the strdup() function for the specified string, making sure that the string to
 allocate is a healthy string (contains NULL character).
*/
char* strdup_safe( const char* str, char* file, int line ) {

  char* new_str;

  if( strlen( str ) > 10000 ) {
    print_output( "Attempting to call strdup for a string exceeding 10000 chars", FATAL, file, line );
    exit( 1 );
  }

  new_str = strdup( str );

  return( new_str );

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

#ifdef HAVE_SYS_TIMES_H
/*!
 \param tm  Pointer to timer structure to clear.

 Clears the total accumulated time in the specified timer structure.
*/
void timer_clear( timer** tm ) {

  if( *tm == NULL ) {
    *tm = (timer*)malloc_safe( sizeof( timer ), __FILE__, __LINE__ ); 
  }

  (*tm)->total = 0;

}

/*!
 \param tm  Pointer to timer structure to start timing.

 Starts the timer to start timing a segment of code.
*/
void timer_start( timer** tm ) {

  if( *tm == NULL ) {
    *tm = (timer*)malloc_safe( sizeof( timer ), __FILE__, __LINE__ );
    timer_clear( tm );
  }

  times( &((*tm)->start) );

}

/*!
 \param tm  Pointer to timer structure to stop timing.

 Stops the specified running counter and totals up the amount
 of user time that has accrued.
*/
void timer_stop( timer** tm ) {

  struct tms tmp;  /* Temporary holder for stop time */

  assert( *tm != NULL );

  times( &tmp );
  (*tm)->total += tmp.tms_utime - (*tm)->start.tms_utime;

}
#endif

/*!
 \param type  Type of functional unit (see \ref func_unit_types for legal values)

 \return Returns a string giving the user-readable name of the given functional unit type
*/
const char* get_funit_type( int type ) {

  const char* type_str;

  if( (type >= 0) && (type < FUNIT_TYPES) ) {
    type_str = funit_types[type];
  } else {
    type_str = funit_types[FUNIT_TYPES];
  }

  return( type_str );

}

/*
 $Log$
 Revision 1.45  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.44  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.43  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.42  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.41  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

 Revision 1.40  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.39  2006/01/16 18:10:20  phase1geo
 Causing all error information to get sent to stderr no matter what mode we
 are in.  Updating error diagnostics for this change.  Full regression now
 passes.

 Revision 1.38  2006/01/14 04:17:23  phase1geo
 Adding is_func_unit function to check to see if a -e value is a valid module, function,
 task or named begin/end block.  Updated regression accordingly.  We are getting closer
 but still have a few diagnostics to figure out yet.

 Revision 1.37  2005/12/19 05:18:24  phase1geo
 Fixing memory leak problems with instance1.1.  Full regression has some segfaults
 that need to be looked at now.

 Revision 1.36  2005/12/14 23:03:24  phase1geo
 More updates to remove memory faults.  Still a work in progress but full
 regression passes.

 Revision 1.35  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.34  2005/12/01 19:46:50  phase1geo
 Removed Tcl/Tk from source files if HAVE_TCLTK is not defined.

 Revision 1.33  2005/11/30 18:25:56  phase1geo
 Fixing named block code.  Full regression now passes.  Still more work to do on
 named blocks, however.

 Revision 1.32  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.

 Revision 1.31  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.30  2005/05/02 15:33:34  phase1geo
 Updates.

 Revision 1.29  2004/04/21 05:14:03  phase1geo
 Adding report_gui checking to print_output and adding error handler to TCL
 scripts.  Any errors occurring within the code will be propagated to the user.

 Revision 1.28  2004/03/22 13:26:52  phase1geo
 Updates for upcoming release.  We are not quite ready to release at this point.

 Revision 1.27  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.26  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.25  2003/10/03 03:08:44  phase1geo
 Modifying filename in summary output to only specify basename of file instead
 of entire path.  The verbose report contains the full pathname still, however.

 Revision 1.24  2003/09/22 19:42:31  phase1geo
 Adding print_output WARNING_WRAP and FATAL_WRAP lines to allow multi-line
 error output to be properly formatted to the output stream.

 Revision 1.23  2003/08/15 20:02:08  phase1geo
 Added check for sys/times.h file for new code additions.

 Revision 1.22  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.21  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.20  2003/02/10 06:08:56  phase1geo
 Lots of parser updates to properly handle UDPs, escaped identifiers, specify blocks,
 and other various Verilog structures that Covered was not handling correctly.  Fixes
 for proper event type handling.  Covered can now handle most of the IV test suite from
 a parsing perspective.

 Revision 1.19  2003/01/04 09:25:15  phase1geo
 Fixing file search algorithm to fix bug where unexpected module that was
 ignored cannot be found.  Added instance7.v diagnostic to verify appropriate
 handling of this problem.  Added tree.c and tree.h and removed define_t
 structure in lexer.

 Revision 1.18  2002/12/06 02:18:59  phase1geo
 Fixing bug with calculating list and concatenation lengths when MBIT_SEL
 expressions were included.  Also modified file parsing algorithm to be
 smarter when searching files for modules.  This change makes the parsing
 algorithm much more optimized and fixes the bug specified in our bug list.
 Added diagnostic to verify fix for first bug.

 Revision 1.17  2002/10/31 23:14:30  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.16  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.15  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.14  2002/10/23 03:39:07  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.13  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.12  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.11  2002/09/25 05:36:08  phase1geo
 Initial version of parameter support is now in place.  Parameters work on a
 basic level.  param1.v tests this basic functionality and param1.cdd contains
 the correct CDD output from handling parameters in this file.  Yeah!

 Revision 1.10  2002/07/20 22:22:52  phase1geo
 Added ability to create implicit signals for local signals.  Added implicit1.v
 diagnostic to test for correctness.  Full regression passes.  Other tweaks to
 output information.

 Revision 1.9  2002/07/20 20:48:09  phase1geo
 Fixing a bug that caused the same file to be added to the use_files list
 more than once.  A filename will only appear once in this list now.  Updates
 to the TODO list.

 Revision 1.8  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.7  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.6  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.5  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

