#ifndef __OBFUSCATE_H__
#define __OBFUSCATE_H__

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
/*@shared@*/ char* obfuscate_name( const char* real_name, char prefix );

/*! \brief Deallocates all memory associated with obfuscation */
void obfuscate_dealloc();

#endif

