#ifndef __KEYWORDS_H__
#define __KEYWORDS_H__

/*!
 \file     keywords.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/2/2001
 \brief    Contains functions for checking Verilog keywords.
*/

/*! Returns the defined value of the keyword or IDENTIFIER if this is not a Verilog keyword. */
extern int lexer_keyword_code( const char* str, int length );

#endif

