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
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <ctype.h>
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
#include "obfuscate.h"
#include "profiler.h"
#include "vpi.h"

extern bool        report_gui;
extern bool        flag_use_command_line_debug;
#ifndef VPI_ONLY
#ifdef DEBUG_MODE
#define CLI_DEBUG_MODE_EXISTS 1
extern bool        cli_debug_mode;
#endif
#ifdef HAVE_TCLTK
extern Tcl_Interp* interp;
#endif
#endif

#ifndef CLI_DEBUG_MODE_EXISTS
static bool cli_debug_mode = FALSE;
#endif

/*!
 If set to TRUE, suppresses all non-fatal error messages from being displayed.
*/
static bool output_suppressed;

/*!
 If set to TRUE, causes debug information to be spewed to screen.
*/
bool debug_mode;

/*!
 If set to TRUE, outputs memory information to standard output for processing.
*/
bool test_mode = FALSE;

/*!
 Contains the total number of bytes malloc'ed during the simulation run.  This
 information is output to the user after simulation as a performance indicator.
*/
int64 curr_malloc_size = 0;

/*!
 Holds the largest number of bytes in allocation at one period of time.
*/
int64 largest_malloc_size = 0;

/*!
 Holds some output that will be displayed via the print_output command.  This is
 created globally so that memory does not need to be reallocated for each function
 that wishes to use it.
*/
char user_msg[USER_MSG_LENGTH];

/*!
 Array of functional unit names used for output purposes.
*/
static const char* funit_types[FUNIT_TYPES+1] = { "module", "named block", "function", "task", "no_score", "afunction", "atask", "named block", "UNKNOWN" };


/*!
 \param value Boolean value of suppression.

 Sets the global variable output_suppressed to the specified value.
*/
void set_output_suppression( bool value ) { PROFILE(SET_OUTPUT_SUPPRESSION);

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
 Looks at the user's environment and searches for COVERED_TESTMODE, if the environment variable
 is set, sets the global test_mode variable to TRUE; otherwise, sets it to FALSE.
*/
void set_testmode() {

  test_mode = (getenv( "COVERED_TESTMODE" ) != NULL);

}

/*!
 \param msg   Message to display.
 \param type  Type of message to output
 \param file  Name of file that called this function
 \param line  Line number that this function was called in

 Displays the specified message to standard output based on the type of message
 being output.
*/
void print_output( const char* msg, int type, const char* file, int line ) {

  FILE* outf = debug_mode ? stdout : stderr;
  char  tmpmsg[USER_MSG_LENGTH];

  switch( type ) {
    case DEBUG:
      if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
#ifdef VPI_ONLY
        vpi_print_output( msg );
#else
        unsigned int rv;
        printf( "%s\n", msg );  rv = fflush( stdout );  assert( rv == 0 );
#endif
      }
      break;
    case NORMAL:
      if( !output_suppressed || debug_mode ) {
#ifdef VPI_ONLY
        vpi_print_output( msg );
#else
        printf( "%s\n", msg );
#endif
      }
      break;
    case WARNING:
      if( !output_suppressed ) {
        if( report_gui ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "WARNING!  %s\n", msg );
          assert( rv < USER_MSG_LENGTH );
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
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "WARNING!  %s (file: %s, line: %d)\n", msg, file, line );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
#endif
#endif
        } else {
          fprintf( outf, "    WARNING!  %s (file: %s, line: %d)\n", msg, obf_file( file ), line );
        }
      }
      break;
    case WARNING_WRAP:
      if( !output_suppressed || debug_mode ) {
        if( report_gui ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "              %s\n", msg );
          assert( rv < USER_MSG_LENGTH );
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
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "%s (file: %s, line: %d)\n", msg, file, line );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
          fprintf( stderr, "ERROR!  %s (file: %s, line: %d)\n", msg, obf_file( file ), line );
#endif
#endif
        } else {
          fprintf( stderr, "ERROR!  %s (file: %s, line: %d)\n", msg, obf_file( file ), line );
        }
      } else {
        if( report_gui ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "%s\n", msg );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
          fprintf( stderr, "ERROR!  %s\n", msg );
#endif
#endif
        } else {
          fprintf( stderr, "ERROR!  %s\n", msg );
        }
      }
      break;
    case FATAL_WRAP:
      if( report_gui ) {
        unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "%s\n", msg );
        assert( rv < USER_MSG_LENGTH );
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
 \param argc          Number of arguments in the argv parameter list
 \param argv          List of arguments being parsed
 \param option_index  Index of current option being parsed

 \return Returns TRUE if the specified option has a valid argument; otherwise,
         returns FALSE to indicate that there was an error in parsing the command-line.

 This function is called whenever a command-line argument requires a value.  It verifies
 that a value was specified (however, it does not make sure that the value is
 the correct type).  Outputs an error message and returns a value of FALSE if a value was
 not specified; otherwise, returns TRUE.
*/
bool check_option_value( int argc, const char** argv, int option_index ) { PROFILE(CHECK_OPTION_VALUE);

  bool retval = TRUE;  /* Return value for this function */

  if( ((option_index + 1) >= argc) || (argv[option_index+1][0] == '-') ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Missing option value to the right of the %s option", argv[option_index] );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param token String to check for valid variable name.
 \return Returns TRUE if the specified string is a legal variable name; otherwise,
         returns FALSE.

 If the specified string follows all of the rules for a legal program
 variable (doesn't start with a number, contains only a-zA-Z0-9_ characters),
 returns a value of TRUE; otherwise, returns a value of FALSE.
*/
bool is_variable( const char* token ) { PROFILE(IS_VARIABLE);

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

  PROFILE_END;

  return( retval );

}

/*!
 \param token  Pointer to string to parse.

 \return Returns TRUE if the specified token is a valid argument representing
         a functional unit.
*/
bool is_func_unit( const char* token ) { PROFILE(IS_FUNC_UNIT);

  char* orig;                          /* Temporary string */
  char* rest;                          /* Temporary string */
  char* front;                         /* Temporary string */
  bool  okay = (strlen( token ) > 0);  /* Specifies if this token is a functional unit value or not */

  /* Allocate memory */
  orig  = strdup_safe( token );
  rest  = strdup_safe( token );
  front = strdup_safe( token );

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
  free_safe( orig, (strlen( token ) + 1) );
  free_safe( rest, (strlen( token ) + 1) );
  free_safe( front, (strlen( token ) + 1) );

  PROFILE_END;

  return( okay );

}

/*!
 \param token String to check for valid pathname-ness

 \return Returns TRUE if the specified string would be a legal filename to
         write to; otherwise, returns FALSE.
*/
bool is_legal_filename( const char* token ) { PROFILE(IS_LEGAL_FILENAME);

  bool  retval = FALSE;  /* Return value for this function */
  FILE* tmpfile;         /* Temporary file pointer */

  if( (tmpfile = fopen( token, "w" )) != NULL ) {
    unsigned int rv = fclose( tmpfile );
    assert( rv == 0 );
    retval = TRUE;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param str  String containing pathname to file.

 \return Returns pointer to string containing only base filename.

 Extracts the file basename of the specified filename string.
*/
const char* get_basename( const char* str ) { PROFILE(GET_BASENAME);

  const char* ptr;  /* Pointer to current character in str */

  ptr = (str + strlen( str )) - 1;

  while( (ptr > str) && (*ptr != '/') ) {
    ptr--;
  }

  if( *ptr == '/' ) {
    ptr++;
  }

  PROFILE_END;

  return( ptr );

}

/*!
 \param str  String containing pathname to file.

 \return Returns pointer to string containing only the directory path

 Extracts the directory path from the specified filename (or returns NULL
 if there is no directory path).

 \warning
 Modifies the given string!
*/
char* get_dirname( char* str ) { PROFILE(GET_DIRNAME);

  char* ptr;  /* Pointer to current character in str */

  ptr = (str + strlen( str )) - 1;

  while( (ptr > str) && (*ptr != '/') ) {
    ptr--;
  }

  *ptr = '\0';
  
  PROFILE_END;

  return( str );

}

/*!
 \param dir Name of directory to check for existence.
 \return Returns TRUE if the specified directory exists; otherwise, returns FALSE.

 Checks to see if the specified directory actually exists in the file structure.
 If the directory is found to exist, returns TRUE; otherwise, returns FALSE.
*/
bool directory_exists( const char* dir ) { PROFILE(DIRECTORY_EXISTS);

  bool        retval = FALSE;  /* Return value for this function */
  struct stat filestat;        /* Statistics of specified directory */

  if( stat( dir, &filestat ) == 0 ) {

    if( S_ISDIR( filestat.st_mode ) ) {

      retval = TRUE;

    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param dir        Name of directory to read files from.
 \param ext_head   Pointer to extension list.
 \param file_head  Pointer to head element of filename string list.
 \param file_tail  Pointer to tail element of filename string list.

 \bug Need to order files according to extension first instead of filename.

 \throws anonymous Throw

 Opens the specified directory for reading and loads (in order) all files that
 contain the specified extensions (if ext_head is NULL, load only *.v files).
 Stores all string filenames to the specified string list.
*/
void directory_load(
  const char*     dir,
  const str_link* ext_head,
  str_link**      file_head,
  str_link**      file_tail
) { PROFILE(DIRECTORY_LOAD);

  DIR*            dir_handle;  /* Pointer to opened directory */
  struct dirent*  dirp;        /* Pointer to current directory entry */
  const str_link* curr_ext;    /* Pointer to current extension string */
  char*           ptr;         /* Pointer to current character in filename */
  int             tmpchars;    /* Number of characters needed to store full pathname for file */
  char*           tmpfile;     /* Temporary string holder for full pathname of file */

  if( (dir_handle = opendir( dir )) == NULL ) {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to read directory %s", dir );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  } else {

    unsigned int rv;

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
          unsigned int rv;
          /* Found valid extension, add to list */
          tmpchars = strlen( dirp->d_name ) + strlen( dir ) + 2;
          tmpfile  = (char*)malloc_safe( tmpchars );
          rv = snprintf( tmpfile, tmpchars, "%s/%s", dir, dirp->d_name );
          assert( rv < tmpchars );
          if( str_link_find( tmpfile, *file_head ) == NULL ) {
            (void)str_link_add( tmpfile, file_head, file_tail );
            (*file_tail)->suppl = 0x1;
          } else {
            free_safe( tmpfile, (strlen( tmpfile ) + 1) );
          }
        }
      }
    }

    rv = closedir( dir_handle );
    assert( rv == 0 );

  }

  PROFILE_END;

}

/*!
 \param file Name of file to check for existence.
 \return Returns TRUE if the specified file exists; otherwise, returns FALSE.

 Checks to see if the specified file actually exists in the file structure.
 If the file is found to exist, returns TRUE; otherwise, returns FALSE.
*/
bool file_exists( const char* file ) { PROFILE(FILE_EXISTS);

  bool        retval = FALSE;  /* Return value for this function */
  struct stat filestat;        /* Statistics of specified directory */

  if( stat( file, &filestat ) == 0 ) {

    if( S_ISREG( filestat.st_mode ) || S_ISFIFO( filestat.st_mode ) ) {

      retval = TRUE;

    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param file  File to read next line from.
 \param line  Pointer to string which will contain read line minus newline character.
 \param line_size  Pointer to number of 
 
 \return Returns FALSE if feof is encountered; otherwise, returns TRUE.

 Reads in a single line of information from the specified file and returns a string
 containing the read line to the calling function.
*/
bool util_readline(
  FILE*  file,
  char** line,
  int*   line_size
) { PROFILE(UTIL_READLINE);

  char  c;      /* Character recently read from file */
  int   i = 0;  /* Current index of line */

  *line_size = 128;
  *line      = (char*)malloc_safe( *line_size );

  while( !feof( file ) && ((c = (char)fgetc( file )) != '\n') ) {

    if( i == (*line_size - 1) ) {
      *line_size *= 2;
      *line       = (char*)realloc_safe( *line, (*line_size / 2), *line_size );
    }

    (*line)[i] = c;
    i++;

  }

  if( !feof( file ) ) {
    (*line)[i] = '\0';
  } else {
    free_safe( *line, *line_size );
    *line = NULL;
  }

  PROFILE_END;

  return( !feof( file ) );

}

/*!
 \param value  Input string that will be searched for environment variables

 \return Returns the given value with environment variables substituted in.  This value should
         be freed by the calling function.
 
 \throws anonymous Throw
*/
char* substitute_env_vars(
  const char* value
) { PROFILE(SUBSTITUTE_ENV_VARS);

  char*       newvalue    = NULL;   /* New value */
  int         newvalue_index;       /* Current index into newvalue */
  const char* ptr;                  /* Pointer to current character in value */
  char        env_var[4096];        /* Name of found environment variable */
  int         env_var_index;        /* Current index to write into env_var string */
  bool        parsing_var = FALSE;  /* Set to TRUE when we are parsing an environment variable */
  char*       env_value;            /* Environment variable value */

  ptr            = value;
  newvalue_index = 0;

  Try {

    while( *ptr != '\0' || parsing_var ) {
      if( parsing_var ) {
        if( isalnum( *ptr ) || (*ptr == '_') ) {
          env_var[env_var_index] = *ptr;
          env_var_index++;
        } else {
          env_var[env_var_index] = '\0';
          if( (env_value = getenv( env_var )) != NULL ) {
            newvalue = (char*)realloc_safe( newvalue, (strlen( newvalue ) + 1), (newvalue_index + strlen( env_value ) + 1) );
            strcat( newvalue, env_value );
            newvalue_index += strlen( env_value );
            parsing_var = FALSE;
            ptr--;
          } else {
            unsigned int i;
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown environment variable $%s in string \"%s\"", env_var, value );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
        }
      } else if( *ptr == '$' ) {
        parsing_var   = TRUE;
        env_var_index = 0;
      } else {
        newvalue = (char*)realloc_safe( newvalue, (strlen( newvalue ) + 1), (newvalue_index + 2) );
        newvalue[newvalue_index]   = *ptr;
        newvalue[newvalue_index+1] = '\0';
        newvalue_index++;
      }
      ptr++;
    }

  } Catch_anonymous {
    free_safe( newvalue, (strlen( newvalue ) + 1) );
    Throw 0;
  }

  PROFILE_END;

  return( newvalue );

}

/*!
 \param scope   Full scope to extract from.
 \param front   Highest level of hierarchy extracted.
 \param rest    Hierarchy left after extraction.
 
 Extracts the highest level of hierarchy from the specified scope,
 returning that instance name to the value of front and the the
 rest of the hierarchy in the value of rest.
*/
void scope_extract_front( const char* scope, char* front, char* rest ) { PROFILE(SCOPE_EXTRACT_FRONT);
  
  const char* ptr;      /* Pointer to current character */
  char        endchar;  /* Set to the character we are searching for */
  
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

  PROFILE_END;
  
}

/*!
 \param scope  Full scope to extract from.
 \param back   Lowest level of hierarchy extracted.
 \param rest   Hierarchy left after extraction.
 
 Extracts the lowest level of hierarchy from the specified scope,
 returning that instance name to the value of back and the the
 rest of the hierarchy in the value of rest.
*/
void scope_extract_back( const char* scope, char* back, char* rest ) { PROFILE(SCOPE_EXTRACT_BACK);

  const char* ptr;      /* Pointer to current character */
  char        endchar;  /* Set to the character we are searching for */

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

  PROFILE_END;
  
}

/*!
 \param scope
 \param front
 \param back

*/
void scope_extract_scope( const char* scope, const char* front, char* back ) { PROFILE(SCOPE_EXTRACT_SCOPE);

  back[0] = '\0';

  if( (strncmp( scope, front, strlen( front ) ) == 0) && (strlen( scope ) > strlen( front )) ) {
    strcpy( back, (scope + strlen( front ) + 1) );
  }

  PROFILE_END;

}

/*!
 \param str  String to create printable version of

 \return Returns printable version of the given string (with any escaped sequences removed)

 Allocates memory for and generates a printable version of the given string (a signal or
 instance name).  The calling function is responsible for deallocating the string returned.
*/
char* scope_gen_printable(
  const char* str
) { PROFILE(SCOPE_GEN_PRINTABLE);

  char* new_str;  /* New version of string with escaped sequences removed */

  assert( strlen( obf_sig( str ) ) < 4096 );

  /* Remove escape sequences, if any */
  if( str[0] == '\\' ) {
    char         tmp_str[4096];
    unsigned int rv = sscanf( str, "\\%[^ \n\t\r\b]", tmp_str );
    assert( rv == 1 );
    new_str = strdup_safe( tmp_str );
  } else {
    new_str = strdup_safe( obf_sig( str ) );
  }

  PROFILE_END;

  return( new_str );

} 

/*!
 \param str1  Pointer to signal/instance name
 \param str2  Pointer to signal/instance name

 \return Returns TRUE if the two strings are equal, properly handling the case where one or
         both are escaped names (start with an escape character and end with a space).
*/
bool scope_compare(
  const char* str1,
  const char* str2
) { PROFILE(SCOPE_COMPARE);

  bool  retval;    /* Return value for this function */
  char* new_str1;  /* New form of str1 with escaped sequences removed */
  char* new_str2;  /* New form of str1 with escaped sequences removed */

  /* Create printable versions of the strings */
  new_str1 = scope_gen_printable( str1 );
  new_str2 = scope_gen_printable( str2 );

  /* Perform the compare */
  retval = (strcmp( new_str1, new_str2 ) == 0);

  /* Deallocate the memory */
  free_safe( new_str1, (strlen( new_str1 ) + 1) );
  free_safe( new_str2, (strlen( new_str2 ) + 1) );

  PROFILE_END;

  return( retval );

}

/*
 \param scope  Scope of some signal.

 \return Returns TRUE if specified scope is local to this module (no hierarchy given);
         otherwise, returns FALSE.

 Parses specified scope for '.' character.  If one is found, returns FALSE; otherwise,
 returns TRUE.
*/
bool scope_local( const char* scope ) { PROFILE(SCOPE_LOCAL);

  const char* ptr;             /* Pointer to current character */
  bool        esc;             /* Set to TRUE if current is escaped */
  bool        wspace = FALSE;  /* Set if last character seen was a whitespace */

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

  PROFILE_END;

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
static void convert_file_to_module( char* mname, int len, char* fname ) { PROFILE(CONVERT_FILE_TO_MODULE);

  char* ptr;   /* Pointer to current character in filename */
  char* lptr;  /* Pointer to last character in module name */
  int   i;     /* Loop iterator */

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

  PROFILE_END;

}

  

/*!
 \param curr  Pointer to current file in list.
 \param mod   Name of module searching for.

 \return Returns pointer to next Verilog file to parse or NULL if no files were found.

 Iterates through specified file list, searching for next Verilog file to parse.
 If a file is a library file (suppl field is 'D'), the name of the module to search
 for is compared with the name of the file.
*/
str_link* get_next_vfile( str_link* curr, const char* mod ) { PROFILE(GET_NEXT_VFILE);

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

  /* Specify that the returned file will be parsed */
  if( next != NULL ) {
    next->suppl2 = 1;
  }

  PROFILE_END;

  return( next );

}

/*!
 \param size           Number of bytes to allocate.
 \param file           File that called this function.
 \param line           Line number of file that called this function.
 \param profile_index  Profile index of function that called this function

 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.
*/
void* malloc_safe1( size_t size, /*@unused@*/ const char* file, /*@unused@*/ int line, unsigned int profile_index ) {

  void* obj;  /* Object getting malloc address */

  assert( size <= MAX_MALLOC_SIZE );
  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = malloc( size );
#ifdef TESTMODE
  if( test_mode ) {
    printf( "MALLOC (%p) %d bytes (file: %s, line: %d) - %lld\n", obj, size, file, line, curr_malloc_size );
  }
#endif
  assert( obj != NULL );

  /* Profile the malloc */
  MALLOC_CALL(profile_index);

  return( obj );

}

/*!
 \param size           Number of bytes to allocate.
 \param file           Name of file that called this function.
 \param line           Line number of file that called this function.
 \param profile_index  Profile index of function that called this function

 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.  Unlike
 malloc_safe, there is no upper bound on the amount of memory to allocate.
*/
void* malloc_safe_nolimit1( size_t size, /*@unused@*/ const char* file, /*@unused@*/ int line, unsigned int profile_index ) {

  void* obj;  /* Object getting malloc address */

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = malloc( size );
#ifdef TESTMODE
  if( test_mode ) {
    printf( "MALLOC (%p) %d bytes (file: %s, line: %d) - %lld\n", obj, size, file, line, curr_malloc_size );
  }
#endif
  assert( obj != NULL );

  /* Profile the malloc */
  MALLOC_CALL(profile_index);

  return( obj );

}

/*!
 Safely performs a free function of heap memory.  Also keeps track
 of current memory usage for output information at end of program
 life.
*/
void free_safe1(
  void*        ptr,           /*!< Pointer to object to deallocate */
  unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  if( ptr != NULL ) {
    free( ptr );
  }

  /* Profile the free */
  FREE_CALL(profile_index);

}

/*!
 Safely performs a free function of heap memory.  Also keeps track
 of current memory usage for output information at end of program
 life.
*/
void free_safe2(
  void*        ptr,           /*!< Pointer to object to deallocate */
  size_t       size,          /*!< Number of bytes that will be deallocated */
  const char*  file,          /*!< File that is calling this function */
  int          line,          /*!< Line number in file that is calling this function */
  unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  if( ptr != NULL ) {
    curr_malloc_size -= size;
#ifdef TESTMODE
    if( test_mode ) {
      printf( "FREE (%p) %d bytes (file: %s, line: %d) - %lld\n", ptr, size, file, line, curr_malloc_size );
    }
#endif
    free( ptr );
  }

  /* Profile the free */
  FREE_CALL(profile_index);

}

/*!
 Calls the strdup() function for the specified string, making sure that the string to
 allocate is a healthy string (contains NULL character).
*/
char* strdup_safe1(
               const char*  str,           /*!< String to duplicate */
  /*@unused@*/ const char*  file,          /*!< Name of file that called this function */
  /*@unused@*/ int          line,          /*!< Line number of file that called this function */
               unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  char* new_str;
  int   str_len = strlen( str ) + 1;

  assert( str_len <= MAX_MALLOC_SIZE );
  curr_malloc_size += str_len;
  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }
  new_str = strdup( str );
#ifdef TESTMODE
  if( test_mode ) {
    printf( "STRDUP (%p) %d bytes (file: %s, line: %d) - %lld\n", new_str, str_len, file, line, curr_malloc_size );
  }
#endif
  assert( new_str != NULL );

  /* Profile the malloc */
  MALLOC_CALL(profile_index);

  return( new_str );

}

/*!
 Calls the realloc() function for the specified memory and size, making sure that the memory
 size doesn't exceed a threshold value and that the requested memory was allocated.
*/
void* realloc_safe1(
  /*@null@*/ void*        ptr,           /*!< Pointer to old memory to copy */
             size_t       old_size,      /*!< Size of originally allocated memory (in bytes) */
             size_t       size,          /*!< Size of new allocated memory (in bytes) */
             const char*  file,          /*!< Name of file that called this function */
             int          line,          /*!< Line number of file that called this function */
             unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  void* newptr;

  assert( size <= MAX_MALLOC_SIZE );

  curr_malloc_size -= old_size;
  curr_malloc_size += size;
  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }
 
  if( size == 0 ) {
    if( ptr != NULL ) {
      free( ptr );
    }
    newptr = NULL;
  } else {
    newptr = realloc( ptr, size );
    assert( newptr != NULL );
  }
#ifdef TESTMODE
  if( test_mode ) {
    printf( "REALLOC (%p -> %p) %d (%d) bytes (file: %s, line: %d) - %lld\n", ptr, newptr, size, old_size, file, line, curr_malloc_size );
  }
#endif

  MALLOC_CALL(profile_index);

  return( newptr );

}

/*!
 Creates a string that contains num_spaces number of space characters,
 adding a NULL character at the end of the string to allow for correct
 usage by the strlen and other string functions.
*/
void gen_space(
  /*@out@*/ char* spaces,     /*!< Pointer to string to places spaces into */
            int   num_spaces  /*!< Number of spaces to place in string */
) { PROFILE(GEN_SPACE);

  int i;     /* Loop iterator */

  for( i=0; i<num_spaces; i++ ) {
    spaces[i] = ' ';
  }

  spaces[i] = '\0';

  PROFILE_END;
  
}

#ifdef HAVE_SYS_TIME_H
/*!
 \param tm  Pointer to timer structure to clear.

 Clears the total accumulated time in the specified timer structure.
*/
static void timer_clear( timer** tm ) {

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

  gettimeofday( &((*tm)->start), NULL );

}

/*!
 \param tm  Pointer to timer structure to stop timing.

 Stops the specified running counter and totals up the amount
 of user time that has accrued.
*/
void timer_stop( timer** tm ) {

  struct timeval tmp;  /* Temporary holder for stop time */

  assert( *tm != NULL );

  gettimeofday( &tmp, NULL );
  (*tm)->total += ((tmp.tv_sec - (*tm)->start.tv_sec) * 1000000) + (tmp.tv_usec - (*tm)->start.tv_usec);

}
#endif

/*!
 \param type  Type of functional unit (see \ref func_unit_types for legal values)

 \return Returns a string giving the user-readable name of the given functional unit type
*/
const char* get_funit_type( int type ) { PROFILE(GET_FUNIT_TYPE);

  const char* type_str;

  if( (type >= 0) && (type < FUNIT_TYPES) ) {
    type_str = funit_types[type];
  } else {
    type_str = funit_types[FUNIT_TYPES];
  }

  PROFILE_END;

  return( type_str );

}

/*!
 \param hits     Number of items hit during simulation
 \param total    Number of total items
 \param misses   Pointer to a storage element which will contain the calculated number of items missed during simulation
 \param percent  Pointer to a storage element which will contain the calculated hit percent information

 Calculates the number of misses and hit percentage information from the given hit and total information, storing
 the results in the misses and percent storage elements.

 \note
 If the total number of items is 0, the hit percentage will be calculated as 100% covered.
*/
void calc_miss_percent(
  int    hits,
  int    total,
  int*   misses,
  float* percent
) { PROFILE(CALC_MISS_PERCENT);

  if( total == 0 ) {
    *percent = 100;
  } else {
    *percent = ((hits / (float)total) * 100);
  }

  *misses = (total - hits);

  PROFILE_END;

}


/*
 $Log$
 Revision 1.93  2008/06/23 23:34:51  phase1geo
 Adding more score diagnostics to regression suite for coverage.

 Revision 1.92  2008/05/30 23:00:48  phase1geo
 Fixing Doxygen comments to eliminate Doxygen warning messages.

 Revision 1.91  2008/05/30 05:38:33  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.90.2.1  2008/05/23 14:50:23  phase1geo
 Optimizing vector_op_add and vector_op_subtract algorithms.  Also fixing issue with
 vector set bit.  Updating regressions per this change.

 Revision 1.90  2008/04/10 23:16:42  phase1geo
 Fixing issues with memory and file handling in preprocessor when an error
 occurs (so that we can recover properly in the GUI).  Also fixing various
 GUI issues from the previous checkin.  Working on debugging problem with
 preprocessing code in verilog.tcl.  Checkpointing.

 Revision 1.89  2008/04/08 23:58:17  phase1geo
 Fixing test mode code so that it works properly in regression and stand-alone
 runs.

 Revision 1.88  2008/04/06 12:41:04  phase1geo
 Fixing memory calculation issue with scope_gen_printable.  Updating regressions.

 Revision 1.87  2008/04/06 05:46:54  phase1geo
 Another regression memory deallocation fix.  Updates to regression files.

 Revision 1.86  2008/03/26 21:29:32  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.85  2008/03/22 05:23:24  phase1geo
 More attempts to fix memory problems.  Things are still pretty broke at the moment.

 Revision 1.84  2008/03/18 05:11:28  phase1geo
 More bug fixes for memory handling.

 Revision 1.83  2008/03/18 03:56:44  phase1geo
 More updates for memory checking (some "fixes" here as well).

 Revision 1.82  2008/03/17 22:02:32  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.81  2008/03/17 05:26:17  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.80  2008/03/14 22:00:21  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.79  2008/03/11 22:06:49  phase1geo
 Finishing first round of exception handling code.

 Revision 1.78  2008/02/22 20:39:22  phase1geo
 More updates for exception handling.

 Revision 1.77  2008/01/16 23:10:34  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.76  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.75  2008/01/10 04:59:05  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.74  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.73  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.72  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.71  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.70  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.69  2007/12/11 23:19:14  phase1geo
 Fixed compile issues and completed first pass injection of profiling calls.
 Working on ordering the calls from most to least.

 Revision 1.68  2007/12/10 23:16:22  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.67  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.66  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.65  2007/09/04 22:50:50  phase1geo
 Fixed static_afunc1 issues.  Reran regressions and updated necessary files.
 Also working on debugging one remaining issue with mem1.v (not solved yet).

 Revision 1.64  2007/08/19 02:08:32  phase1geo
 Fixing one syntax error with fix for compiling issue when --enable-debug is
 not specified.

 Revision 1.63  2007/08/17 22:08:45  phase1geo
 Fixing linker issue when --enable-debug is not specified to the configure script.

 Revision 1.62  2007/08/07 02:23:32  phase1geo
 Fixing bug 1687409.

 Revision 1.61  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.60  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.59  2007/04/11 22:29:49  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.58  2007/03/13 22:12:59  phase1geo
 Merging changes to covered-0_5-branch to fix bug 1678931.

 Revision 1.57  2006/12/11 23:29:17  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.

 Revision 1.56.2.1  2007/03/13 22:05:11  phase1geo
 Fixing bug 1678931.  Updated regression.

 Revision 1.56  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.55  2006/08/31 22:32:18  phase1geo
 Things are in a state of flux at the moment.  I have added proper parsing support
 for assertions, properties and sequences.  Also added partial support for the $root
 space (though I need to work on figuring out how to handle it in terms of the
 instance tree) and all that goes along with that.  Add parsing support with an
 error message for multi-dimensional array declarations.  Regressions should not be
 expected to run correctly at the moment.

 Revision 1.54  2006/08/18 22:32:57  phase1geo
 Adding get_dirname routine to util.c for future use.

 Revision 1.53  2006/08/18 22:19:54  phase1geo
 Fully integrated obfuscation into the development release.

 Revision 1.52  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.51  2006/08/18 04:41:14  phase1geo
 Incorporating bug fixes 1538920 and 1541944.  Updated regressions.  Only
 event1.1 does not currently pass (this does not pass in the stable version
 yet either).

 Revision 1.50  2006/08/06 04:36:20  phase1geo
 Fixing bugs 1533896 and 1533827.  Also added -rI option that will ignore
 the race condition check altogether (has not been verified to this point, however).

 Revision 1.49  2006/05/03 21:17:49  phase1geo
 Finishing assertion source code viewer functionality.  We just need to add documentation
 to the GUI user's guide and we should be set here (though we may want to consider doing
 some syntax highlighting at some point).

 Revision 1.48  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.47  2006/04/07 22:31:07  phase1geo
 Fixes to get VPI to work with VCS.  Getting close but still some work to go to
 get the callbacks to start working.

 Revision 1.46  2006/04/07 03:47:50  phase1geo
 Fixing run-time issues with VPI.  Things are running correctly now with IV.

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

