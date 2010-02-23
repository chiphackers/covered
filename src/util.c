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
 \file     util.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

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
#endif /* HAVE_STRING_H */
#ifndef RUNLIB
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
#include <tcl.h>
#include <tk.h> 
#endif /* HAVE_TCLTK */
#endif /* VPI_ONLY */
#endif /* RUNLIB */

#ifdef HAVE_MPATROL
#include <mpdebug.h>
#endif /* HAVE_MPATROL */

#include "defines.h"
#include "util.h"
#include "link.h"
#include "obfuscate.h"
#include "profiler.h"
#include "vpi.h"

extern bool        flag_use_command_line_debug;
#ifndef RUNLIB
#ifndef VPI_ONLY
#ifdef DEBUG_MODE
#define CLI_DEBUG_MODE_EXISTS 1
extern bool        cli_debug_mode;
#endif /* DEBUG_MODE */
#ifdef HAVE_TCLTK
extern Tcl_Interp* interp;
#else
static const char* interp = NULL;
#endif /* HAVE_TCLTK */
#else
static const char* interp = NULL;
#endif /* VPI_ONLY */
#else
static const char* interp = NULL;
#endif /* RUNLIB */

#ifndef CLI_DEBUG_MODE_EXISTS
static bool cli_debug_mode = FALSE;
#endif /* CLI_DEBUG_MODE_EXISTS */

/*!
 If set to TRUE, suppresses all non-warning/fatal error messages from being displayed.
*/
bool quiet_mode;

/*!
 If set to TRUE, suppresses all normal messages from being displayed.
*/
bool terse_mode;

/*!
 If set to TRUE, suppresses all warnings from being displayed.
*/
bool warnings_suppressed = FALSE;

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
 Sets the global quiet_mode variable to the specified value.
*/
void set_quiet(
  bool value  /*!< Boolean value of suppression */
) {

  quiet_mode = value;

}

/*!
 Set the global terse_mode variable to the given value.
*/
void set_terse(
  bool value  /*!< Boolean value of terse mode */
) {

  terse_mode = value;

}

/*!
 Sets the global debug mode to the specified value.
*/
void set_debug(
  bool value  /*!< Boolean value of debug mode */
) {

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
 Displays the specified message to standard output based on the type of message
 being output.
*/
void print_output(
  const char* msg,   /*!< Message to display */
  int         type,  /*!< Type of message to output (see \ref output_type for legal values) */
  const char* file,  /*!< Name of file that called this function */
  int         line   /*!< Line number that this function was called in */
) {

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
    case HEADER:
      if( !quiet_mode || terse_mode || debug_mode ) {
#ifdef VPI_ONLY
        vpi_print_output( msg );
#else
        printf( "%s\n", msg );
#endif
      }
      break;
    case NORMAL:
      if( (!quiet_mode && !terse_mode) || debug_mode ) {
#ifdef VPI_ONLY
        vpi_print_output( msg );
#else
        printf( "%s\n", msg );
#endif
      }
      break;
    case WARNING:
      if( (!quiet_mode || terse_mode) && !warnings_suppressed ) {
#ifndef RUNLIB
        if( interp != NULL ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "WARNING!  %s\n", msg );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
#endif /* HAVE_TCLTK */
#endif /* VPI_ONLY */
        } else {
#endif /* RUNLIB */
          fprintf( outf, "    WARNING!  %s\n", msg );
#ifndef RUNLIB
        }
#endif /* RUNLIB */
      } else if( debug_mode ) {
#ifndef RUNLIB
        if( interp != NULL ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "WARNING!  %s (file: %s, line: %d)\n", msg, file, line );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
#endif /* HAVE_TCLTK */
#endif /* VPI_ONLY */
        } else {
#endif /* RUNLIB */
          fprintf( outf, "    WARNING!  %s (file: %s, line: %d)\n", msg, obf_file( file ), line );
#ifndef RUNLIB
        }
#endif /* RUNLIB */
      }
      break;
    case WARNING_WRAP:
      if( ((!quiet_mode || terse_mode) && !warnings_suppressed) || debug_mode ) {
#ifndef RUNLIB
        if( interp != NULL ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "              %s\n", msg );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_AppendElement( interp, tmpmsg );
#endif /* HAVE_TCLTK */
#endif /* VPI_ONLY */
        } else {
#endif /* RUNLIB */
          fprintf( outf, "              %s\n", msg );
#ifndef RUNLIB
        }
#endif /* RUNLIB */
      }
      break; 
    case FATAL:
      (void)fflush( stdout );
      if( debug_mode ) {
#ifndef RUNLIB
        if( interp != NULL ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "%s (file: %s, line: %d)\n", msg, file, line );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
          fprintf( stderr, "ERROR!  %s (file: %s, line: %d)\n", msg, obf_file( file ), line );
#endif /* HAVE_TCLTK */
#endif /* VPI_ONLY */
        } else {
#endif /* RUNLIB */
          fprintf( stderr, "ERROR!  %s (file: %s, line: %d)\n", msg, obf_file( file ), line );
#ifndef RUNLIB
        }
#endif /* RUNLIB */
      } else {
#ifndef RUNLIB
        if( interp != NULL ) {
          unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "%s\n", msg );
          assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
          Tcl_SetResult( interp, tmpmsg, TCL_VOLATILE );
          fprintf( stderr, "ERROR!  %s\n", msg );
#endif /* HAVE_TCLTK */
#endif /* VPI_ONLY */
        } else {
#endif /* RUNLIB */
          fprintf( stderr, "ERROR!  %s\n", msg );
#ifndef RUNLIB
        }
#endif
      }
      break;
    case FATAL_WRAP:
#ifndef RUNLIB
      if( interp != NULL ) {
        unsigned int rv = snprintf( tmpmsg, USER_MSG_LENGTH, "%s\n", msg );
        assert( rv < USER_MSG_LENGTH );
#ifndef VPI_ONLY
#ifdef HAVE_TCLTK
        Tcl_AppendElement( interp, tmpmsg );
#endif /* HAVE_TCLTK */
#endif /* VPI_ONLY */
      } else { 
#endif /* RUNLIB */
        fprintf( stderr, "        %s\n", msg );
#ifndef RUNLIB
      }
#endif /* RUNLIB */
      break;
    default:  break;
  }

}

/*!
 \return Returns TRUE if the specified option has a valid argument; otherwise,
         returns FALSE to indicate that there was an error in parsing the command-line.

 This function is called whenever a command-line argument requires a value.  It verifies
 that a value was specified (however, it does not make sure that the value is
 the correct type).  Outputs an error message and returns a value of FALSE if a value was
 not specified; otherwise, returns TRUE.
*/
bool check_option_value(
  int          argc,         /*!< Number of arguments in the argv parameter list */
  const char** argv,         /*!< List of arguments being parsed */
  int          option_index  /*!< Index of current option being parsed */
) { PROFILE(CHECK_OPTION_VALUE);

  bool retval = TRUE;  /* Return value for this function */

  if( ((option_index + 1) >= argc) || ((argv[option_index+1][0] == '-') && (strlen(argv[option_index+1]) > 1)) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Missing option value to the right of the %s option", argv[option_index] );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the specified string is a legal variable name; otherwise,
         returns FALSE.

 If the specified string follows all of the rules for a legal program
 variable (doesn't start with a number, contains only a-zA-Z0-9_ characters),
 returns a value of TRUE; otherwise, returns a value of FALSE.
*/
bool is_variable(
  const char* token  /*!< String to check for valid variable name */
) { PROFILE(IS_VARIABLE);

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
 \return Returns TRUE if the specified token is a valid argument representing
         a functional unit.
*/
bool is_func_unit(
  const char* token  /*!< Pointer to string to parse */
) { PROFILE(IS_FUNC_UNIT);

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
 \return Returns TRUE if the specified string would be a legal filename to
         write to; otherwise, returns FALSE.
*/
bool is_legal_filename(
  const char* token  /*!< String to check for valid pathname-ness */
) { PROFILE(IS_LEGAL_FILENAME);

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
 \return Returns pointer to string containing only base filename.

 Extracts the file basename of the specified filename string.
*/
const char* get_basename(
  const char* str  /*!< String containing pathname to file */
) { PROFILE(GET_BASENAME);

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
 \return Returns pointer to string containing only the directory path

 Extracts the directory path from the specified filename (or returns NULL
 if there is no directory path).

 \warning
 Modifies the given string!
*/
char* get_dirname(
  char* str  /*!< String containing pathname to file */
) { PROFILE(GET_DIRNAME);

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
 \return Returns absolute path of the given absolute pathname to the current path.
*/
char* get_absolute_path(
  const char* filename  /*!< Filename to get the absolute pathname of */
) { PROFILE(GET_ABSOLUTE_PATH);

  char*        abs_path = NULL;
  char*        tmp;
  char*        dir;
  char         this_cwd[4096];
  char*        srv;
  unsigned int irv;

  /* Get a copy of the filename and calculate its directory and basename */
  tmp = strdup_safe( filename );
  dir = get_dirname( tmp );

  /* Get the original working directory so that we can return there */
  srv = getcwd( this_cwd, 4096 );
  assert( srv != NULL );

  /* If we have a directory to go to, change to the directory */
  if( dir[0] != '\0' ) {

    char         cwd[4096];
    unsigned int slen;
    char*        file = dir + strlen( dir ) + 1;

    /* Change to the specified directory */
    irv = chdir( dir );
    assert( irv == 0 );

    /* Get the current working directory and create the absolute path */
    srv = getcwd( cwd, 4096 );
    assert( srv != NULL );

    slen     = strlen( cwd ) + strlen( file ) + 2;
    abs_path = (char*)malloc_safe( slen );
    irv      = snprintf( abs_path, slen, "%s/%s", cwd, file );
    assert( irv < slen );

    /* Return to the original directory */
    irv = chdir( this_cwd );
    assert( irv == 0 );

  /* Otherwise, the file is in this directory */
  } else {

    unsigned int slen;

    slen = strlen( this_cwd ) + strlen( filename ) + 2;

    abs_path = (char*)malloc_safe( slen );
    irv      = snprintf( abs_path, slen, "%s/%s", this_cwd, filename );
    assert( irv < slen );

  }

  /* Deallocate used memory */
  free_safe( tmp, (strlen( filename ) + 1) );

  PROFILE_END;

  return( abs_path );

}

/*!
 \return Returns relative path of the given absolute pathname to the current path.
*/
char* get_relative_path(
  const char* abs_path  /*!< Absolute pathname of file to get relative path for */
) { PROFILE(GET_RELATIVE_PATH);

  char*        rel_path = NULL;
  char         cwd[4096];
  char*        rv;
  unsigned int i;

  /* Get the current working directory */
  rv = getcwd( cwd, 4096 );
  assert( rv != NULL );

  /*
   Compare the absolute path to the current working directory path and stop when we see a
   miscompare or run into the end of a path string.
  */
  i = 0;
  while( (i < strlen( cwd )) && (i < strlen( abs_path )) && (abs_path[i] == cwd[i]) ) i++;

  /* We should have never gotten to the end of the absolute path */
  assert( i < strlen( abs_path ) );

  /*
   If the current working directory is completely a part of the absolute path, the relative pathname
   is beneath the current working directory.
  */
  if( i == strlen( cwd ) ) {
    rel_path = strdup_safe( abs_path + i + 1 );

  /*
   Otherwise, we need to back up and go forward.
  */
  } else {

    unsigned int save_i;
    char         trel[4096];

    /* Find the previous backslash */
    while( (i > 0) && (cwd[i] != '/') ) i--;
    assert( cwd[i] == '/' );
    
    /* Save the current position of i */
    save_i = i + 1; 

    /* Create back portion of path */
    trel[0] = '\0';
    for( ; i<strlen( cwd ); i++ ) {
      if( cwd[i] == '/' ) {
        strcat( trel, "../" );
      }
    }

    /* Now append the absolute path */
    strcat( trel, (abs_path + save_i) );

    /* Finally, make a copy of the calculated relative path */
    rel_path = strdup_safe( trel );

  }

  PROFILE_END;

  return( rel_path );

}

/*!
 \return Returns TRUE if the specified directory exists; otherwise, returns FALSE.

 Checks to see if the specified directory actually exists in the file structure.
 If the directory is found to exist, returns TRUE; otherwise, returns FALSE.
*/
bool directory_exists(
  const char* dir  /*!< Name of directory to check for existence */
) { PROFILE(DIRECTORY_EXISTS);

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
 \bug Need to order files according to extension first instead of filename.

 \throws anonymous Throw

 Opens the specified directory for reading and loads (in order) all files that
 contain the specified extensions (if ext_head is NULL, load only *.v files).
 Stores all string filenames to the specified string list.
*/
void directory_load(
  const char*     dir,        /*!< Name of directory to read files from */
  const str_link* ext_head,   /*!< Pointer to extension list */
  str_link**      file_head,  /*!< Pointer to head element of filename string list */
  str_link**      file_tail   /*!< Pointer to tail element of filename string list */
) { PROFILE(DIRECTORY_LOAD);

  DIR*            dir_handle;  /* Pointer to opened directory */
  struct dirent*  dirp;        /* Pointer to current directory entry */
  const str_link* curr_ext;    /* Pointer to current extension string */
  char*           ptr;         /* Pointer to current character in filename */
  unsigned int    tmpchars;    /* Number of characters needed to store full pathname for file */
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
 \return Returns TRUE if the specified file exists; otherwise, returns FALSE.

 Checks to see if the specified file actually exists in the file structure.
 If the file is found to exist, returns TRUE; otherwise, returns FALSE.
*/
bool file_exists(
  const char* file  /*!< Name of file to check for existence */
) { PROFILE(FILE_EXISTS);

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
 \return Returns FALSE if feof is encountered; otherwise, returns TRUE.

 Reads in a single line of information from the specified file and returns a string
 containing the read line to the calling function.
*/
bool util_readline(
            FILE*         file,      /*!< File to read next line from */
  /*@out@*/ char**        line,      /*!< Pointer to string which will contain read line minus newline character */
  /*@out@*/ unsigned int* line_size  /*!< Pointer to number of characters allocated for line */
) { PROFILE(UTIL_READLINE);

  char         c;      /* Character recently read from file */
  unsigned int i = 0;  /* Current index of line */

  if( *line == NULL ) {
    *line_size = 128;
    *line      = (char*)malloc_safe( *line_size );
  }

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
 \return Returns TRUE if a quoted string was properly parsed; otherwise, returns FALSE.

 Parses a double-quoted string from the file pointer if one exists.  Removes quotes.
*/
bool get_quoted_string(
            FILE* file,  /*!< Pointer to file to parse */
  /*@out@*/ char* line   /*!< User supplied character array to hold quoted string */
) { PROFILE(GET_QUOTED_STRING);

  bool found = FALSE;  /* Return value for this function */
  char c[128];         /* Temporary whitespace storage */
  int  i     = 0;      /* Loop iterator */

  /* First, remove any whitespace and temporarily store it */
  while( ((c[i] = getc( file )) != EOF) && isspace( c[i] ) ) i++;

  /* If the character we are looking at is a double-quote, continue parsing */
  if( c[i] == '"' ) {

    i = 0;
    while( ((line[i] = getc( file )) != EOF) && (line[i] != '"') ) i++;
    line[i]  = '\0';
    found = TRUE;

  /* Otherwise, ungetc the collected characters */
  } else {

    for( ; i >= 0; i-- ) {
      (void)ungetc( c[i], file );
    }

  }

  PROFILE_END;

  return( found );

}

/*!
 \return Returns the given value with environment variables substituted in.  This value should
         be freed by the calling function.
 
 \throws anonymous Throw
*/
char* substitute_env_vars(
  const char* value  /*!< Input string that will be searched for environment variables */
) { PROFILE(SUBSTITUTE_ENV_VARS);

  char*       newvalue      = NULL;   /* New value */
  int         newvalue_index;         /* Current index into newvalue */
  const char* ptr;                    /* Pointer to current character in value */
  char        env_var[4096];          /* Name of found environment variable */
  int         env_var_index = 0;      /* Current index to write into env_var string */
  bool        parsing_var   = FALSE;  /* Set to TRUE when we are parsing an environment variable */
  char*       env_value;              /* Environment variable value */

  newvalue       = (char*)malloc_safe( 1 );
  newvalue[0]    = '\0';
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
 Extracts the highest level of hierarchy from the specified scope,
 returning that instance name to the value of front and the the
 rest of the hierarchy in the value of rest.
*/
void scope_extract_front(
            const char* scope,  /*!< Full scope to extract from */
  /*@out@*/ char*       front,  /*!< Highest level of hierarchy extracted */
  /*@out@*/ char*       rest    /*!< Hierarchy left after extraction */
) { PROFILE(SCOPE_EXTRACT_FRONT);
  
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
 Extracts the lowest level of hierarchy from the specified scope,
 returning that instance name to the value of back and the the
 rest of the hierarchy in the value of rest.
*/
void scope_extract_back(
            const char* scope,  /*!< Full scope to extract from */
  /*@out@*/ char*       back,   /*!< Lowest level of hierarchy extracted */
  /*@out@*/ char*       rest    /*!< Hierarchy left after extraction */
) { PROFILE(SCOPE_EXTRACT_BACK);

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
 Parses the given scope and removes the front portion of this scope (if the front portion of the scope
 matches the beginning portion of scope) and returns the remaining scope in the array pointed to by back.
 If front does not exist within scope, back is set to a value of the null string.  Assumes that the length
 of back is allocated and large enough to hold the full value of scope, if necessary.
*/
void scope_extract_scope(
            const char* scope,  /*!< Full scope to search */
            const char* front,  /*!< Leading portion of scope to exclude */
  /*@out@*/ char*       back    /*!< Following portion of scope that is in scope that is not in front */
) { PROFILE(SCOPE_EXTRACT_SCOPE);

  back[0] = '\0';

  if( (strncmp( scope, front, strlen( front ) ) == 0) && (strlen( scope ) > strlen( front )) ) {
    strcpy( back, (scope + strlen( front ) + 1) );
  }

  PROFILE_END;

}

/*!
 \return Returns printable version of the given string (with any escaped sequences removed)

 Allocates memory for and generates a printable version of the given string (a signal or
 instance name).  The calling function is responsible for deallocating the string returned.
*/
char* scope_gen_printable(
  const char* str  /*!< String to create printable version of */
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
 \return Returns TRUE if the two strings are equal, properly handling the case where one or
         both are escaped names (start with an escape character and end with a space).
*/
bool scope_compare(
  const char* str1,  /*!< Pointer to signal/instance name */
  const char* str2   /*!< Pointer to signal/instance name */
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
 \return Returns TRUE if specified scope is local to this module (no hierarchy given);
         otherwise, returns FALSE.

 Parses specified scope for '.' character.  If one is found, returns FALSE; otherwise,
 returns TRUE.
*/
bool scope_local(
  const char* scope  /*!< Scope of some signal */
) { PROFILE(SCOPE_LOCAL);

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
 Takes in a filename (with possible directory information and/or possible extension)
 and transforms it into a filename with the directory and extension information stripped
 off.  Much like the the functionality of the unix command "basename".  Returns the 
 stripped filename in the mname parameter.
*/
static void convert_file_to_module(
  char* mname,  /*!< Name of module extracted */
  int   len,    /*!< Length of mname string (we cannot exceed this value) */
  char* fname   /*!< Name of filename to extract module name from */
) { PROFILE(CONVERT_FILE_TO_MODULE);

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
 \return Returns pointer to next Verilog file to parse or NULL if no files were found.

 Iterates through specified file list, searching for next Verilog file to parse.
 If a file is a library file (suppl field is 'D'), the name of the module to search
 for is compared with the name of the file.
*/
str_link* get_next_vfile(
  str_link*   curr,  /*!< Pointer to current file in list */
  const char* mod    /*!< Name of module searching for */
) { PROFILE(GET_NEXT_VFILE);

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
 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.
*/
void* malloc_safe1(
               size_t       size,          /*!< Number of bytes to allocate */
  /*@unused@*/ const char*  file,          /*!< File that called this function */
  /*@unused@*/ int          line,          /*!< Line number of file that called this function */
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  void* obj;  /* Object getting malloc address */

  assert( size <= MAX_MALLOC_SIZE );

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = malloc( size );
#ifdef TESTMODE
  if( test_mode ) {
    printf( "MALLOC (%p) %d bytes (file: %s, line: %d) - %" FMT64 "d\n", obj, (int)size, file, line, curr_malloc_size );
  }
#endif
  assert( obj != NULL );

  /* Profile the malloc */
  MALLOC_CALL(profile_index);

  return( obj );

}

/*!
 \return Pointer to allocated memory.

 Allocated memory like a malloc() call but performs some pre-allocation and
 post-allocation checks to be sure that the malloc call works properly.  Unlike
 malloc_safe, there is no upper bound on the amount of memory to allocate.
*/
void* malloc_safe_nolimit1(
               size_t       size,          /*!< Number of bytes to allocate */
  /*@unused@*/ const char*  file,          /*!< Name of file that called this function */
  /*@unused@*/ int          line,          /*!< Line number of file that called this function */
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  void* obj;  /* Object getting malloc address */

  curr_malloc_size += size;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = malloc( size );
#ifdef TESTMODE
  if( test_mode ) {
    printf( "MALLOC (%p) %d bytes (file: %s, line: %d) - %" FMT64 "d\n", obj, (int)size, file, line, curr_malloc_size );
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
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
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
  /*@unused@*/ const char*  file,          /*!< File that is calling this function */
  /*@unused@*/ int          line,          /*!< Line number in file that is calling this function */
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  if( ptr != NULL ) {
    curr_malloc_size -= size;
#ifdef TESTMODE
    if( test_mode ) {
      printf( "FREE (%p) %d bytes (file: %s, line: %d) - %" FMT64 "d\n", ptr, (int)size, file, line, curr_malloc_size );
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
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
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
    printf( "STRDUP (%p) %d bytes (file: %s, line: %d) - %" FMT64 "d\n", new_str, str_len, file, line, curr_malloc_size );
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
  /*@null@*/   void*        ptr,           /*!< Pointer to old memory to copy */
               size_t       old_size,      /*!< Size of originally allocated memory (in bytes) */
               size_t       size,          /*!< Size of new allocated memory (in bytes) */
  /*@unused@*/ const char*  file,          /*!< Name of file that called this function */
  /*@unused@*/ int          line,          /*!< Line number of file that called this function */
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
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
    printf( "REALLOC (%p -> %p) %d (%d) bytes (file: %s, line: %d) - %" FMT64 "d\n", ptr, newptr, (int)size, (int)old_size, file, line, curr_malloc_size );
  }
#endif

  MALLOC_CALL(profile_index);

  return( newptr );

}

/*!
 Calls the realloc() function for the specified memory and size, making sure that the memory
 size doesn't exceed a threshold value and that the requested memory was allocated.
*/
void* realloc_safe_nolimit1(
  /*@null@*/   void*        ptr,           /*!< Pointer to old memory to copy */
               size_t       old_size,      /*!< Size of originally allocated memory (in bytes) */
               size_t       size,          /*!< Size of new allocated memory (in bytes) */
  /*@unused@*/ const char*  file,          /*!< Name of file that called this function */
  /*@unused@*/ int          line,          /*!< Line number of file that called this function */
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  void* newptr;

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
    printf( "REALLOC (%p -> %p) %d (%d) bytes (file: %s, line: %d) - %" FMT64 "d\n", ptr, newptr, (int)size, (int)old_size, file, line, curr_malloc_size );
  }
#endif

  MALLOC_CALL(profile_index);

  return( newptr );

}

/*!
 \return Returns a pointer to the newly allocated/initialized data

 Verifies that the specified size is not oversized, callocs the memory, verifies that the memory pointer returned is not NULL,
 and performs some memory statistic handling.
*/
void* calloc_safe1(
               size_t       num,           /*!< Number of elements to allocate */
               size_t       size,          /*!< Size of each element that is allocated */
  /*@unused@*/ const char*  file,          /*!< Name of file that called this function */
  /*@unused@*/ int          line,          /*!< Line number of file that called this function */
  /*@unused@*/ unsigned int profile_index  /*!< Profile index of function that called this function */
) {

  void*  obj;
  size_t total = (num * size);

  assert( total > 0 );

  curr_malloc_size += total;

  if( curr_malloc_size > largest_malloc_size ) {
    largest_malloc_size = curr_malloc_size;
  }

  obj = calloc( num, size );
#ifdef TESTMODE
  if( test_mode ) {
    printf( "CALLOC (%p) %d bytes (file: %s, line: %d) - %" FMT64 "d\n", obj, (int)total, file, line, curr_malloc_size );
  }
#endif
  assert( obj != NULL );

  MALLOC_CALL(profile_index);

  return( obj );

}

/*!
 Creates a string that contains num_chars number of characters specified by
 the value of c, adding a NULL character at the end of the string to allow
 for correct usage by the strlen and other string functions.
*/
void gen_char_string(
  /*@out@*/ char* str,       /*!< Pointer to string to places spaces into */
            char  c,         /*!< Character to write */
            int   num_chars  /*!< Number of spaces to place in string */
) { PROFILE(GEN_SPACE);

  int i;     /* Loop iterator */

  for( i=0; i<num_chars; i++ ) {
    str[i] = c;
  }

  str[i] = '\0';

  PROFILE_END;
  
}

/*!
 \return Returns a pointer to the modified string (or NULL if the string only contains
         underscores).
*/
char* remove_underscores(
  char* str  /*!< String to remove underscore characters from */
) { PROFILE(REMOVE_UNDERSCORES);

  char*        start = NULL;
  unsigned int i;
  unsigned int cur   = 1;

  for( i=0; i<strlen( str ); i++ ) {
    if( str[i] != '_' ) {
      if( start == NULL ) {
        start = str + i;
      } else {
        start[cur++] = str[i];
      }
    }
  }

  if( start != NULL ) {
    start[cur] = '\0';
  }

  PROFILE_END;

  return( start );

}

#ifdef HAVE_SYS_TIME_H
/*!
 Clears the total accumulated time in the specified timer structure.
*/
void timer_clear(
  timer** tm  /*!< Pointer to timer structure to clear */
) {

  if( *tm == NULL ) {
    *tm = (timer*)malloc_safe( sizeof( timer ) );
  }

  (*tm)->total = 0;

}

/*!
 Starts the timer to start timing a segment of code.
*/
void timer_start(
  timer** tm  /*!< Pointer to timer structure to start timing */
 ) {

  if( *tm == NULL ) {
    *tm = (timer*)malloc_safe( sizeof( timer ) );
    timer_clear( tm );
  }

  gettimeofday( &((*tm)->start), NULL );

}

/*!
 Stops the specified running counter and totals up the amount
 of user time that has accrued.
*/
void timer_stop(
  timer** tm  /*!< Pointer to timer structure to stop timing */
) {

  struct timeval tmp;  /* Temporary holder for stop time */

  assert( *tm != NULL );

  gettimeofday( &tmp, NULL );
  (*tm)->total += ((tmp.tv_sec - (*tm)->start.tv_sec) * 1000000) + (tmp.tv_usec - (*tm)->start.tv_usec);

}

/*!
 \return Returns a user-readable version of the timer structure.
*/
char* timer_to_string(
  timer* tm  /*!< Pointer to timer structure */
) {

  static char str[33];  /* Minimal amount of space needed to store the current time */

#ifdef TESTMODE
  strcpy( str, "NA" );
#else
  /* If the time is less than a minute, output the seconds and milliseconds */
  if( tm->total < 10 ) {
    /*@-duplicatequals -formattype -formatcode@*/
    unsigned int rv = snprintf( str, 33, "0.00000%1" FMT64 "u seconds", tm->total );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < 33 );
  } else if( tm->total < 100 ) {
    /*@-duplicatequals -formattype -formatcode@*/
    unsigned int rv = snprintf( str, 33, "0.0000%1" FMT64 "u seconds", (tm->total / 10) ); 
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < 33 );
  } else if( tm->total < 1000 ) {
    /*@-duplicatequals -formattype -formatcode@*/
    unsigned int rv = snprintf( str, 33, "0.000%1" FMT64 "u seconds", (tm->total / 100) );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < 33 );
  } else if( tm->total < 60000000 ) {
    /*@-duplicatequals -formattype -formatcode@*/
    unsigned int rv = snprintf( str, 33, "%2" FMT64 "u.%03" FMT64 "u seconds", (tm->total / 1000000), ((tm->total % 1000000) / 1000) );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < 33 );
  } else if( tm->total < 3600000000LL ) {
    /*@-duplicatequals -formattype -formatcode@*/
    unsigned int rv = snprintf( str, 33, "%2" FMT64 "u minutes, %2" FMT64 "u seconds", (tm->total / 60000000), ((tm->total % 60000000) / 1000000) );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < 33 );
  } else {
    /*@-duplicatequals -formattype -formatcode@*/
    unsigned int rv = snprintf( str, 33, "%2llu hours, %2llu minutes, %2" FMT64 "u seconds", 
                                (tm->total / 3600000000LL), ((tm->total % 3600000000LL) / 60000000), ((tm->total % 60000000) / 1000000) );
    /*@=duplicatequals =formattype =formatcode@*/
    assert( rv < 33 );
  }
#endif

  return( str );

}
#endif

/*!
 \return Returns a string giving the user-readable name of the given functional unit type
*/
const char* get_funit_type(
  int type  /*!< Type of functional unit (see \ref func_unit_types for legal values) */
) { PROFILE(GET_FUNIT_TYPE);

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
 Calculates the number of misses and hit percentage information from the given hit and total information, storing
 the results in the misses and percent storage elements.

 \note
 If the total number of items is 0, the hit percentage will be calculated as 100% covered.
*/
void calc_miss_percent(
  int    hits,    /*!< Number of items hit during simulation */
  int    total,   /*!< Number of total items */
  int*   misses,  /*!< Pointer to a storage element which will contain the calculated number of items missed during simulation */
  float* percent  /*!< Pointer to a storage element which will contain the calculated hit percent information */
) { PROFILE(CALC_MISS_PERCENT);

  if( total == 0 ) {
    *percent = 100;
  } else {
    *percent = ((hits / (float)total) * 100);
  }

  *misses = (total - hits);

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw substitute_env_vars

 Parses the given file specified by the '-f' option to one of Covered's commands which can contain
 any command-line arguments.  Performs environment variable substitution to any $... variables that
 are found in the file.
*/
void read_command_file(
            const char* cmd_file,  /*!< Name of file to read commands from */
  /*@out@*/ char***     arg_list,  /*!< List of arguments found in specified command file */
  /*@out@*/ int*        arg_num    /*!< Number of arguments in arg_list array */
) { PROFILE(READ_COMMAND_FILE);

  str_link* head    = NULL;  /* Pointer to head element of arg list */
  str_link* tail    = NULL;  /* Pointer to tail element of arg list */
  FILE*     cmd_handle;      /* Pointer to command file */
  char      tmp_str[4096];   /* Temporary holder for read argument */
  str_link* curr;            /* Pointer to current str_link element */
  int       tmp_num = 0;     /* Temporary argument number holder */
  bool      use_stdin;       /* If set to TRUE, uses stdin for the cmd_handle */

  /* Figure out if we should use stdin */
  use_stdin = (strcmp( "-", cmd_file ) == 0);

  if( use_stdin || file_exists( cmd_file ) ) {

    if( (cmd_handle = (use_stdin ? stdin : fopen( cmd_file, "r" ))) != NULL ) {

      unsigned int rv;

      Try {

        while( get_quoted_string( cmd_handle, tmp_str ) || fscanf( cmd_handle, "%s", tmp_str ) == 1 ) {
          (void)str_link_add( substitute_env_vars( tmp_str ), &head, &tail );
          tmp_num++;
        }

      } Catch_anonymous {
        rv = fclose( cmd_handle );
        assert( rv == 0 );
        str_link_delete_list( head );
        Throw 0;
      }

      rv = fclose( cmd_handle );
      assert( rv == 0 );

      /* Set the argument list number now */
      *arg_num = tmp_num;

      /*
       If there were any arguments found in the file, create an argument list and pass it to the
       command-line parser.
      */
      if( tmp_num > 0 ) {

        /* Create argument list */
        *arg_list = (char**)malloc_safe( sizeof( char* ) * tmp_num );
        tmp_num   = 0;

        curr = head;
        while( curr != NULL ) {
          (*arg_list)[tmp_num] = strdup_safe( curr->str );
          tmp_num++;
          curr = curr->next;
        } 
        
        /* Delete list */
        str_link_delete_list( head );
        
      } 
        
    } else {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open command file %s for reading", cmd_file );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Command file %s does not exist", cmd_file );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
        
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the given string is a legal, non-X/Z value.

 Converts a VCD string value to a legal unsigned 64-bit integer value.
*/
bool convert_str_to_uint64(
  const char* str,   /*!< String version of value */
  int         msb,   /*!< MSB offset */
  int         lsb,   /*!< LSB offset */
  uint64*     value  /*!< 64-bit unsigned integer value */
) { PROFILE(CONVERT_STR_TO_UINT64);

  bool        legal   = TRUE;
  const char* str_lsb = (str + (strlen( str ) - 1));
  const char* str_msb = ((str_lsb - msb) < str) ? str : (str_lsb - msb);
  const char* ptr     = str_lsb - lsb;
  uint64      i       = 1;

  /* Make sure that the LSB that was specified is not greater than the string length */
  assert( ptr >= str_lsb );

  *value = 0;

  while( (ptr >= str_msb) && legal && (i > 0) ) {
    *value |= (*ptr == '1') ? i : 0;
    legal   = (*ptr != 'x') && (*ptr != 'z');
    ptr--;
    i <<= 1;
  } 

  PROFILE_END;

  return( legal );

}

/*!
 \return Returns an allocated string version of the integer value.
*/
char* convert_int_to_str(
  int value  /*!< Integer value to convert */
) { PROFILE(CONVERT_INT_TO_STR);

  char         numstr[50];
  unsigned int rv;
  char*        str;
  
  rv = snprintf( numstr, 50, "%d", value );
  assert( rv < 50 );

  str = strdup_safe( numstr );

  PROFILE_END;

  return( str );

}

/*!
 \return Returns the number of bits that are required to store the number of values represented.
*/
int calc_num_bits_to_store(
  int values  /*!< Number of values that need to be stored */
) { PROFILE(CALC_NUM_BITS_TO_STORE);

  unsigned int bits = 1;

  while( (1 << bits) < values ) bits++;

  PROFILE_END;

  return( bits );

}
