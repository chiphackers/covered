#ifndef __OBFUSCATE_H__
#define __OBFUSCATE_H__

/*
 Copyright (c) 2006 Trevor Williams

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
 \file     obfuscate.h
 \author   Trevor Williams  (phase1geo@gmail.com)
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


/*! \brief Sets the global 'obf_mode' variable to the specified value */
void obfuscate_set_mode( bool value );

/*! \brief Gets an obfuscated name for the given actual name */
char* obfuscate_name( const char* real_name, char prefix );

/*! \brief Deallocates all memory associated with obfuscation */
void obfuscate_dealloc();


/*
 $Log$
 Revision 1.6  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.5  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.4  2006/08/18 22:19:54  phase1geo
 Fully integrated obfuscation into the development release.

 Revision 1.3  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.1.2.2  2006/08/18 04:50:51  phase1geo
 First swag at integrating name obfuscation for all output (with the exception
 of CDD output).

 Revision 1.1.2.1  2006/08/17 04:17:38  phase1geo
 Adding files to obfuscate actual names when outputting any user-visible
 information.
*/

#endif
