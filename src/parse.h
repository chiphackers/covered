#ifndef __PARSE_H__
#define __PARSE_H__

/*!
 \file     parse.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
 \brief    Contains functions for parsing Verilog modules
*/

#include "defines.h"
#include "link.h"


//! Parses the specified design and generates scoring modules.
bool parse_design( char* top, char* output_db );

//! Parses VCD dumpfile and scores design.
bool parse_and_score_dumpfile( char* top, char* db, char* vcd );

#endif

