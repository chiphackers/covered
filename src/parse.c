/*!
 \file     parse.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
*/

#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "parse.h"
#include "link.h"
#include "util.h"
#include "db.h"
#include "binding.h"
#include "vcd.h"

extern void reset_lexer( str_link* file_list_head );
extern int VLparse();
extern int VLdebug;

extern str_link* use_files_head;

extern str_link* modlist_head;
extern str_link* modlist_tail;

extern char user_msg[USER_MSG_LENGTH];
extern int  error_count;

/*!
 \param file  Pointer to file to read
 \param line  Read line from specified file.
 \param size  Maximum number of characters that can be stored in line.
 \return Returns the number of characters read from this line.

 Reads from specified file, one character at a time, until either a newline
 or EOF is encountered.  Returns the read line and the number of characters
 stored in the line.  No newline character is added to the line.
*/
int parse_readline( FILE* file, char* line, int size ) {

  int i = 0;  /* Loop iterator */

  while( (i < (size - 1)) && !feof( file ) && ((line[i] = fgetc( file )) != '\n') ) {
    i++;
  }

  if( !feof( file ) ) {
    line[i] = '\0';
  }

  if( i == size ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Line too long.  Must be less than %d characters.", size );
    print_output( user_msg, FATAL );
  }

  return( !feof( file ) );

}

/*!
 \param top        Name of top-level module to score
 \param output_db  Name of output directory for generated scored files.

 \return Returns TRUE if parsing is successful; otherwise, returns FALSE.

 Resets the lexer and parses all Verilog files specified in use_files list.
 After all design files are parsed, their information will be appropriately
 stored in the associated lists.
*/
bool parse_design( char* top, char* output_db ) {

  bool retval = TRUE;  /* Return value of this function */

  str_link_add( top, &modlist_head, &modlist_tail );

  if( use_files_head != NULL ) {

    /* Initialize lexer with first file */
    reset_lexer( use_files_head );

    /* Starting parser */
    if( (VLparse() != 0) || (error_count > 0) ) {
      print_output( "Error in parsing design", FATAL );
      exit( 1 );
    }

    print_output( "========  Completed design parsing  ========\n", DEBUG );

    bind();
  
  } else {

    print_output( "No Verilog input files specified", FATAL );
    retval = FALSE;

  }

  /* Write contents to baseline database file. */
  if( !db_write( output_db, TRUE ) ) {
    print_output( "Unable to write database file", FATAL );
    exit( 1 );
  }

  snprintf( user_msg, USER_MSG_LENGTH, "========  Design written to database %s successfully  ========\n\n", output_db );
  print_output( user_msg, DEBUG );

  return( retval );

}

/*!
 \param db   Name of output database file to generate.
 \param vcd  Name of VCD file to parse for scoring.
 \return Returns TRUE if VCD parsing and scoring is successful; otherwise,
         returns FALSE.

*/
bool parse_and_score_dumpfile( char* db, char* vcd ) {

  bool retval = TRUE;  /* Return value of this function */

  snprintf( user_msg, USER_MSG_LENGTH, "========  Reading in database %s  ========\n", db );
  print_output( user_msg, DEBUG );

  /* Read in contents of specified database file */
  if( !db_read( db, READ_MODE_MERGE_NO_MERGE ) ) {
    print_output( "Unable to read database file", FATAL );
    exit( 1 );
  }
  
  sim_add_statics();

  /* Read in contents of VCD file */
  if( vcd == NULL ) {
    print_output( "VCD file not specified on command line", FATAL );
    exit( 1 );
  }

  snprintf( user_msg, USER_MSG_LENGTH, "========  Reading in VCD dumpfile %s  ========\n", vcd );
  print_output( user_msg, DEBUG );
  
  /* Perform the parse */
  vcd_parse( vcd );
    
  /* Flush any pending statement trees that are waiting for delay */
  db_do_timestep( -1 );

  snprintf( user_msg, USER_MSG_LENGTH, "========  Writing database %s  ========\n", db );
  print_output( user_msg, DEBUG );

  /* Write contents to database file */
  if( !db_write( db, FALSE ) ) {
    print_output( "Unable to write database file", FATAL );
    exit( 1 );
  }

  return( retval );

}

/*
 $Log$
 Revision 1.16  2002/12/01 06:37:52  phase1geo
 Adding appropriate error output and causing score command to proper exit
 if parser errors are found.

 Revision 1.15  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.14  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.13  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.12  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.11  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.10  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.9  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.

 Revision 1.8  2002/07/20 21:34:58  phase1geo
 Separating ability to parse design and score dumpfile.  Now both or either
 can be done (allowing one to parse once and score multiple times).

 Revision 1.7  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.6  2002/07/13 04:09:18  phase1geo
 Adding support for correct implementation of `ifdef, `else, `endif
 directives.  Full regression passes.

 Revision 1.5  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.4  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

