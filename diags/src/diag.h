#ifndef __DIAG_H__
#define __DIAG_H__

/*!
 \file     diag.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/27/2006
 \brief    Contains functions for reading in and running a single diagnostic.
*/


#include "run_cmd.h"


#ifndef __DIAG_S__
#define __DIAG_S__
struct diag_s;
typedef struct diag_s diag;
#endif

#ifndef __RUN_CMD_S__
#define __RUN_CMD_S__
struct run_cmd_s;
typedef struct run_cmd_s run_cmd;
#endif

/*!
 Structure used to store information about a diagnostic.
*/
struct diag_s {
  char*    name;           /*!< Name of diagnostic being run */
  run_cmd* rc_head;        /*!< Head of run command list */
  run_cmd* rc_tail;        /*!< Tail of run command list */
  int      ran;            /*!< Specifies if this diagnostic was run */
  int      failed;         /*!< Specifies if this diagnostic was run and failed */
  list*    failed_cmds;    /*!< Specifies the failing commands */
  diag*    next;           /*!< Pointer to the next diagnostic in the list */
};
  

/*! \brief Allocates, initializes and adds the specified diagnostic to the diagnostic list. */
diag* diag_add( char* diag_name );

/*! \brief Reads the specified diagnostic regression information and stores for later use. */
int read_diag_info( diag* d );

/*! \brief Runs the specified diagnostic, returnings its run-time status. */
void run_current_diag( diag* d );

/*! \brief Outputs the run-time results after all simulation has completed. */
void diag_output_results();

/*! \brief Deallocates all memory allocated for the diagnostic list. */
void diag_dealloc_list();


/*
 $Log$
 Revision 1.2  2006/03/03 23:24:53  phase1geo
 Fixing C-based run script.  This is now working for all but one diagnostic to this
 point.  There is still some work to do here, however.

 Revision 1.1  2006/02/27 23:22:10  phase1geo
 Working on C-version of run command.  Initial version only -- does not work
 at this point.

*/

#endif

