#ifndef __RUN_CMD_H__
#define __RUN_CMD_H__

/*!
 \file     run_cmd.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/27/2006
 \brief    Contains functions for adding to, organizing and deallocating run-time commands
*/


#include "list.h"


typedef struct run_cmd_s {
  char*             step;     /*!< Specifies what step this command is for (SIM, SCORE, etc.) */
  char*             cmd;      /*!< Specifies the command string to run */
  list*             inputs;   /*!< Specifies the list of inputs needed for this command */
  list*             outputs;  /*!< Specifies the list of output needed for this command */
  int               error;    /*!< Specifies if this command is supposed to produce and error */
  struct run_cmd_s* next;     /*!< Pointer to next run command in the list */
} run_cmd;


/*! \brief Adds the current line to the run-time command list */
void run_cmd_add( char* line );

/*! \brief Deallocates the run-time command list */
void run_cmd_dealloc_list();


/*
 $Log$
*/

#endif

