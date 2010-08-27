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
 \file     info.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/12/2003
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "db.h"
#include "defines.h"
#include "info.h"
#include "link.h"
#include "score.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern str_link*    merge_in_head;
extern str_link*    merge_in_tail;
extern int          merge_in_num;
extern char*        merged_file;
extern uint64       num_timesteps;
extern char*        cdd_message;
extern char         user_msg[USER_MSG_LENGTH];
extern unsigned int inline_comb_depth;


/*!
 Informational line for the CDD file.
*/
isuppl info_suppl = {0};

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
int cdd_version = CDD_VERSION;

/*!
 Specifes the pathname where the score command was originally run from.
*/
char score_run_path[4096];

/*!
 Pointer to the head of the score arguments list.
*/
/*@null@*/ str_link* score_args_head = NULL;

/*!
 Pointer to the tail of the score arguments list.
*/
/*@null@*/ str_link* score_args_tail = NULL;


/*!
 *  Adds the specified argument to the list of score arguments that will be written to the CDD file.
 *  */
void score_add_args(
             const char* arg1,  /*!< First argument from score command */
  /*@null@*/ const char* arg2   /*!< Second argument from score command */
) { PROFILE(SCORE_ADD_ARGS);

  str_link* arg    = score_args_head;
  bool      done   = FALSE;
  bool      nondup = ((strncmp( arg1, "-vpi", 4 ) == 0) ||
                      (strncmp( arg1, "-lxt", 4 ) == 0) ||
                      (strncmp( arg1, "-vcd", 4 ) == 0) ||
                      (strncmp( arg1, "-t",   2 ) == 0) ||
                      (strncmp( arg1, "-i",   2 ) == 0) ||
                      (strncmp( arg1, "-o",   2 ) == 0));

  while( !done ) {

    /* Check to see if the specified arguments already exist */
    while( (arg != NULL) && (strcmp( arg->str, arg1 ) != 0) ) {
      arg = arg->next;
    }

    /* If the argument doesn't exist, just add it and be done */
    if( arg == NULL ) {
      arg = str_link_add( strdup_safe( arg1 ), &score_args_head, &score_args_tail );
      if( arg2 != NULL ) {
        arg->str2 = strdup_safe( arg2 );
      }
      done = TRUE;

    /* If the first option exists and its either a non-duplicatible option or it already exists, be done */
    } else if( nondup || ((arg2 != NULL) && (strcmp( arg2, arg->str2 ) == 0)) ) {
      done = TRUE;

    /* Otherwise, advance the arg pointer */
    } else {
      arg = arg->next;
    }

  }

}

/*!
 Sets the vector element size in the global info_suppl structure based on the current machine
 unsigned long byte size.
*/
void info_set_vector_elem_size() { PROFILE(INFO_SET_VECTOR_ELEM_SIZE);

  switch( sizeof( ulong ) ) {
    case 1 :  info_suppl.part.vec_ul_size = 0;  break;
    case 2 :  info_suppl.part.vec_ul_size = 1;  break;
    case 4 :  info_suppl.part.vec_ul_size = 2;  break;
    case 8 :  info_suppl.part.vec_ul_size = 3;  break;
    default:
      print_output( "Unsupported unsigned long size", FATAL, __FILE__, __LINE__ );
      Throw 0;
      /*@-unreachable@*/
      break;
      /*@=unreachable@*/
  }

  PROFILE_END;

}

/*!
 Sets the scored bit in the information supplemental field.
*/
void info_set_scored() { PROFILE(INFO_SET_SCORED);

  /* Indicate that this CDD contains scored information */
  info_suppl.part.scored = 1;

  PROFILE_END;

}

/*!
 Writes information line to specified file.
*/
void info_db_write(
  FILE* file  /*!< Pointer to file to write information to */
) { PROFILE(INFO_DB_WRITE);

  str_link* arg;

  assert( db_list[curr_db]->leading_hier_num > 0 );

  /* Calculate vector element size */
  info_set_vector_elem_size();

  /*@-formattype -formatcode -duplicatequals@*/
  fprintf( file, "%d %x %" FMT32 "x %" FMT64 "u %u %x %s\n",
           DB_TYPE_INFO,
           CDD_VERSION,
           info_suppl.all,
           num_timesteps,
           db_list[curr_db]->inst_num,
           inline_comb_depth,
           db_list[curr_db]->leading_hierarchies[0] );
  /*@=formattype =formatcode =duplicatequals@*/

  /* Display score arguments */
  fprintf( file, "%d %s", DB_TYPE_SCORE_ARGS, score_run_path );

  arg = score_args_head;
  while( arg != NULL ) {
    if( arg->str2 != NULL ) {
      fprintf( file, " 2 %s (%s)", arg->str, arg->str2 );
    } else {
      fprintf( file, " 1 %s", arg->str );
    }
    arg = arg->next;
  }

  fprintf( file, "\n" );

  /* Display the CDD message, if there is one */
  if( cdd_message != NULL ) {
    fprintf( file, "%d %s\n", DB_TYPE_MESSAGE, cdd_message );
  }

  /* Display the merged CDD information, if there are any */
  if( db_list[curr_db]->leading_hier_num == merge_in_num ) {
    str_link*    strl = merge_in_head;
    unsigned int i    = 0;
    while( strl != NULL ) {
      if( strl->suppl < 2 ) {
        if( ((merged_file == NULL) || (strcmp( strl->str, merged_file ) != 0)) && (strl->suppl == 1) ) {
          fprintf( file, "%d %s %s\n", DB_TYPE_MERGED_CDD, strl->str, db_list[curr_db]->leading_hierarchies[i++] );
        } else {
          i++;
        }
      }
      strl = strl->next; 
    }
  } else { 
    str_link*    strl = merge_in_head;
    unsigned int i    = 1; 
    assert( (db_list[curr_db]->leading_hier_num - 1) == merge_in_num );
    while( strl != NULL ) {
      if( strl->suppl < 2 ) {
        if( ((merged_file == NULL) || (strcmp( strl->str, merged_file ) != 0)) && (strl->suppl == 1) ) {
          fprintf( file, "%d %s %s\n", DB_TYPE_MERGED_CDD, strl->str, db_list[curr_db]->leading_hierarchies[i++] );
        } else {
          i++;
        }
      }
      strl = strl->next;
    }
  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw

 \return Returns TRUE if the CDD file read should continue; otherwise, stop reading in the current CDD.

 Reads information line from specified string and stores its information, and creates a new database in
 the database list.
*/
bool info_db_read(
  /*@out@*/ char** line,      /*!< Pointer to string containing information line to parse */
            int    read_mode  /*!< Type of read being performed */
) { PROFILE(INFO_DB_READ);

  int          chars_read;  /* Number of characters scanned in from this line */
  uint32       scored;      /* Indicates if this file contains scored data */
  unsigned int version;     /* Contains CDD version from file */
  char         tmp[4096];   /* Temporary string */
  isuppl       info   = info_suppl;
  bool         retval = TRUE;
  unsigned int inst_num;

  /* Save off original scored value */
  scored = info_suppl.part.scored;

  if( sscanf( *line, "%x%n", &version, &chars_read ) == 1 ) {

    *line = *line + chars_read;

    if( version != CDD_VERSION ) {
      print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }

    /*@-formattype -formatcode -duplicatequals@*/
    if( sscanf( *line, "%" FMT32 "x %" FMT64 "u %u %x %s%n", &(info.all), &num_timesteps, &inst_num, &inline_comb_depth, tmp, &chars_read ) == 5 ) {
    /*@=formattype =formatcode =duplicatequals@*/

      *line = *line + chars_read;

      /* If this CDD contains useful information, continue on */
      if( (info.part.scored != 0) || (read_mode != READ_MODE_MERGE_NO_MERGE) ) {

        /* Create a new database element */
        (void)db_create();

        /* Set the instance number if this CDD has not been scored yet */
        if( db_list[curr_db]->insts == NULL ) {
          db_list[curr_db]->inst_num = inst_num;
          db_list[curr_db]->insts    = (funit_inst**)malloc_safe( sizeof( funit_inst* ) * inst_num );
        }

        /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
        if( (db_list[curr_db]->leading_hier_num > 0) && (strcmp( db_list[curr_db]->leading_hierarchies[0], tmp ) != 0) ) {
          db_list[curr_db]->leading_hiers_differ = TRUE;
        }

        /* Assign this hierarchy to the leading hierarchies array */
        db_list[curr_db]->leading_hierarchies = (char**)realloc_safe( db_list[curr_db]->leading_hierarchies, (sizeof( char* ) * db_list[curr_db]->leading_hier_num), (sizeof( char* ) * (db_list[curr_db]->leading_hier_num + 1)) );
        db_list[curr_db]->leading_hierarchies[db_list[curr_db]->leading_hier_num] = strdup_safe( tmp );
        db_list[curr_db]->leading_hier_num++;

        /* Save off the info supplemental information */
        info_suppl.all = info.all;

        /* Set scored flag to correct value */
        if( info_suppl.part.scored == 0 ) {
          info_suppl.part.scored = scored;
        }

      } else {

        merge_in_num--;
        retval = FALSE;

      }

    } else {

      print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
      Throw 0;

    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \throws anonymous Throw

 Reads score command-line args line from specified string and stores its information.
*/
void args_db_read(
  char** line  /*!< Pointer to string containing information line to parse */
) { PROFILE(ARGS_DB_READ);

  int  chars_read;  /* Number of characters scanned in from this line */
  char tmp1[4096];  /* Temporary string */
  char tmp2[4096];  /* Temporary string */
  int  arg_num;

  if( sscanf( *line, "%s%n", score_run_path, &chars_read ) == 1 ) {

    *line = *line + chars_read;

    /* Store score command-line arguments */
    while( sscanf( *line, "%d%n", &arg_num, &chars_read ) == 1 ) {
      *line = *line + chars_read;
      if( (arg_num == 1) && (sscanf( *line, "%s%n", tmp1, &chars_read ) == 1) ) {
        score_add_args( tmp1, NULL );
      } else if( (arg_num == 2) && (sscanf( *line, "%s (%[^)])%n", tmp1, tmp2, &chars_read ) == 2) ) {
        score_add_args( tmp1, tmp2 );
      }
      *line = *line + chars_read;
    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Read user-specified message from specified string and stores its information.
*/
void message_db_read(
  char** line  /*!< Pointer to string containing information line to parse */
) { PROFILE(MESSAGE_DB_READ);

  /* All we need to do is copy the message */
  if( (cdd_message == NULL) && (strlen( *line + 1 ) > 0) ) {
    cdd_message = strdup_safe( *line + 1 );
  }

  PROFILE_END;

}

/*!
 Parses given line for merged CDD information and stores this information in the appropriate global variables.
*/
void merged_cdd_db_read(
  char** line  /*!< Pointer to string containing merged CDD line to parse */
) { PROFILE(MERGED_CDD_DB_READ);

  char tmp1[4096];  /* Temporary string */
  char tmp2[4096];  /* Temporary string */
  int  chars_read;  /* Number of characters read */

  if( sscanf( *line, "%s %s%n", tmp1, tmp2, &chars_read ) == 2 ) {

    str_link* strl;

    *line = *line + chars_read;

    /* Add merged file */
    if( (strl = str_link_find( tmp1, merge_in_head)) == NULL ) {

      strl = str_link_add( strdup_safe( tmp1 ), &merge_in_head, &merge_in_tail );
      strl->suppl = 1;
      merge_in_num++;

      /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
      if( strcmp( db_list[curr_db]->leading_hierarchies[0], tmp2 ) != 0 ) {
        db_list[curr_db]->leading_hiers_differ = TRUE;
      }

      /* Add its hierarchy */
      db_list[curr_db]->leading_hierarchies = (char**)realloc_safe( db_list[curr_db]->leading_hierarchies, (sizeof( char* ) * db_list[curr_db]->leading_hier_num), (sizeof( char* ) * (db_list[curr_db]->leading_hier_num + 1)) );
      db_list[curr_db]->leading_hierarchies[db_list[curr_db]->leading_hier_num] = strdup_safe( tmp2 );
      db_list[curr_db]->leading_hier_num++;

    } else if( merge_in_num > 0 ) {

      char* file = get_relative_path( tmp1 );
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "File %s in CDD file has been specified on the command-line", file );
      assert( rv < USER_MSG_LENGTH );
      free_safe( file, (strlen( file ) + 1) );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Deallocates all memory associated with the database information section.  Needs to be called
 when the database is closed.
*/
void info_dealloc() { PROFILE(INFO_DEALLOC);

  str_link_delete_list( score_args_head );
  score_args_head = NULL;
  score_args_tail = NULL;

  /* Free merged arguments */
  str_link_delete_list( merge_in_head );
  merge_in_head = NULL;
  merge_in_tail = NULL;
  merge_in_num  = 0;

  /* Free user message */
  free_safe( cdd_message, (strlen( cdd_message ) + 1) );
  cdd_message = NULL;

  PROFILE_END;

}

