#ifndef __RUN_CMD_H__
#define __RUN_CMD_H__

/*!
 \file     run_cmd.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/27/2006
 \brief    Contains functions for adding to, organizing and deallocating run-time commands
*/


#include "list.h"
#include "diag.h"


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

struct run_cmd_s {
  int               step;      /*!< Specifies what step this command is for (SIM, SCORE, etc.) */
  char*             cmd;       /*!< Specifies the command string to run */
  list*             inputs;    /*!< Specifies the list of inputs needed for this command */
  list*             outputs;   /*!< Specifies the list of output needed for this command */
  int               error;     /*!< Specifies if this command is supposed to produce and error */
  int               start;     /*!< Specifies if this is a starting run command */
  int               okay;      /*!< Specifies that this command is okay to run */
  int               executed;  /*!< Set to a value of 1 when this run command has been executed */
  struct run_cmd_s* next;      /*!< Pointer to next run command in the list */
};


/*! \brief Adds the current line to the run-time command list */
void run_cmd_add( char* line, run_cmd** rc_head, run_cmd** rc_tail );

/*! \brief Executes the specified run command */
int run_cmd_execute( run_cmd* rc, diag* d );

/*! \brief Removes all output files generated from the given run command */
void run_cmd_remove_outputs( run_cmd* rc );

/*! \brief Deallocates the run-time command list */
void run_cmd_dealloc_list( run_cmd* rc_head );


/*
 $Log$
 Revision 1.3  2006/03/15 22:48:28  phase1geo
 Updating run program.  Fixing bugs in statement_connect algorithm.  Updating
 regression files.

 Revision 1.2  2006/03/03 23:24:53  phase1geo
 Fixing C-based run script.  This is now working for all but one diagnostic to this
 point.  There is still some work to do here, however.

 Revision 1.1  2006/02/27 23:22:10  phase1geo
 Working on C-version of run command.  Initial version only -- does not work
 at this point.

*/

#endif

