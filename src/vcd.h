#ifndef __VCD_H__
#define __VCD_H__

/*!
 \file     vcd.h
 \author   Trevor Williams (trevorw@charter.net)
 \date     7/21/2002
 \brief    Contains VCD parser functions.
*/

//! Parses specified VCD file, storing information into database.
void vcd_parse( char* vcd_file );

/*
 $Log$
 Revision 1.1  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.
*/

#endif

