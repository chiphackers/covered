#ifndef __OBFUSCATE_H__
#define __OBFUSCATE_H__

/*!
 \file     obfuscate.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/16/2006
 \brief    Contains functions for internal obfuscation.
*/


/*!
 Used for obfuscating signal names.  Improves performance when obfuscation mode is not turned on.
*/
#define obf_sig(x) (obf_mode ? obfuscate_name(x,'s') : x)

/*!
 Used for obfuscating functional unit names.  Improves performance when obfuscation mode is not
 turned on.
*/
#define obf_funit(x) (obf_mode ? obfuscate_name(x,'f') : x)

/*!
 Used for obfuscating file names.  Improves performance when obfuscation mode is not turned on.
*/
#define obf_file(x) (obf_mode ? obfuscate_name(x,'v') : x)

/*!
 Used for obfuscating instance names.  Improves performance when obfuscation mode is not turned on.
*/
#define obf_inst(x) (obf_mode ? obfuscate_name(x,'i') : x)


extern bool obf_mode;


/*! \brief Gets an obfuscated name for the given actual name */
char* obfuscate_name( char* real_name, char prefix );

/*! \brief Deallocates all memory associated with obfuscation */
void obfuscate_dealloc();


/*
 $Log$
 Revision 1.1.2.2  2006/08/18 04:50:51  phase1geo
 First swag at integrating name obfuscation for all output (with the exception
 of CDD output).

 Revision 1.1.2.1  2006/08/17 04:17:38  phase1geo
 Adding files to obfuscate actual names when outputting any user-visible
 information.

*/

#endif
