#ifndef __INFO_H__
#define __INFO_H__

/*!
 \file     info.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/12/2003
 \brief    Contains functions for reading/writing info line of CDD file.
*/

#include <stdio.h>


/*! \brief  Initializes all information variables. */
void info_initialize();

/*! \brief  Writes info line to specified CDD file. */
void info_db_write( FILE* file );

/*! \brief  Reads info line from specified line and stores information. */
bool info_db_read( char** line );


/*
 $Log$
 Revision 1.1  2003/02/12 14:56:27  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

*/

#endif

