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

#ifdef HAVE_MPATROL
#include <mpdebug.h>
#endif

#include "defines.h"
#include "util.h"
#include "link.h"

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

 Displays the specified message to standard output based on the type of message
 being output.
*/
void print_output( char* msg, int type ) {

  switch( type ) {
    case DEBUG:
      if( debug_mode ) { printf( "%s\n", msg ); }
      break;
    case NORMAL:
      if( !output_suppressed || debug_mode ) { printf( "%s\n", msg ); }
      break;
    case WARNING:
      if( !output_suppressed || debug_mode ) { fprintf( stderr, "    WARNING!  %s\n", msg ); }
      break;
    case WARNING_WRAP:
      if( !output_suppressed || debug_mode ) { fprintf( stderr, "              %s\n", msg ); }
      break; 
    case FATAL:
      fprintf( stderr, "ERROR!  %s\n", msg );
      break;
    case FATAL_WRAP:
      fprintf( stderr, "        %s\n", msg );
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
    print_output( user_msg, FATAL );
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
          tmpfile  = (char*)malloc_safe( tmpchars );
          snprintf( tmpfile, tmpchars, "%s/%s", dir, dirp->d_name );
          if( str_link_find( tmpfile, *file_head ) == NULL ) {
            str_link_add( tmpfile, file_head, file_tail );
            (*file_tail)->suppl = 0x1;
          }
        }
      }
    }

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

void scope_extract_scope( char* scope, char* front, char* back ) {

  char* last_ptr = '\0';   /* Pointer to last dot seen */
  char* ptr1     = scope;  /* Pointer to scope         */
  char* ptr2     = front;  /* Pointer to front         */

  back[0]  = '\0';

  if( strncmp( scope, front, strlen( front ) ) == 0 ) {
    strcpy( back, (scope + strlen( front ) + 1) );
  }

}

/*
 \param scope  Scope of some signal.

 \return Returns TRUE if specified scope is local to this module (no hierarchy given);
         otherwise, returns FALSE.

 Parses specified scope for '.' character.  If one is found, returns FALSE; otherwise,
 returns TRUE.
*/
bool scope_local( char* scope ) {

  char* ptr;             /* Pointer to current character                */
  bool  esc;             /* Set to TRUE if current is escaped           */
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

 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.
*/
void* malloc_safe( size_t size ) {

  void* obj;      /* Object getting malloc address */

  if( size > 10000 ) {
    print_output( "Allocating memory chunk larger than 10000 bytes.  Possible error.", WARNING );
    /* printf( "  Memory block size request: %ld bytes\n", (long) size ); */
    assert( size <= 10000 );
  } else if( size <= 0 ) {
    print_output( "Internal:  Attempting to allocate memory of size <= 0", FATAL );
    assert( size > 0 );
  }

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = malloc( size );
  
  if( obj == NULL ) {
    print_output( "Out of heap memory", FATAL );
    exit( 1 );
  }

  return( obj );

}

/*!
 \param size  Number of bytes to allocate.

 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.  Unlike
 malloc_safe, there is no upper bound on the amount of memory to allocate.
*/
void* malloc_safe_nolimit( size_t size ) {

  void* obj;  /* Object getting malloc address */

  if( size <= 0 ) {
    print_output( "Internal:  Attempting to allocate memory of size <= 0", FATAL );
    assert( size > 0 );
  }

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

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

  /* curr_malloc_size -= sizeof( *ptr ); */

  /* printf( "Freeing memory, addr: 0x%lx\n", ptr ); */

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

#ifdef HAVE_SYS_TIMES_H
/*!
 \param tm  Pointer to timer structure to clear.

 Clears the total accumulated time in the specified timer structure.
*/
void timer_clear( timer** tm ) {

  if( *tm == NULL ) {
    *tm = (timer*)malloc_safe( sizeof( timer ) ); 
  }

  (*tm)->total = 0;

}

/*!
 \param tm  Pointer to timer structure to start timing.

 Starts the timer to start timing a segment of code.
*/
void timer_start( timer** tm ) {

  if( *tm == NULL ) {
    *tm = (timer*)malloc_safe( sizeof( timer ) );
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

/*
 $Log$
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

