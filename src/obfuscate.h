#ifndef __OBFUSCATE_H__
#define __OBFUSCATE_H__

/*!
 \file     obfuscate.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/16/2006
 \brief    Contains functions for internal obfuscation.
*/


/*! \brief Gets an obfuscated name for the given actual name */
char* obfuscate_get_name( char* real_name, char prefix );

/*! \brief Deallocates all memory associated with obfuscation */
void obfuscate_dealloc();


/*
 $Log$
 Revision 1.1.2.1  2006/08/17 04:17:38  phase1geo
 Adding files to obfuscate actual names when outputting any user-visible
 information.

*/

#endif
