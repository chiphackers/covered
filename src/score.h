#ifndef __SCORE_H__
#define __SCORE_H__

/*!
 \file     score.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/29/2001
 \brief    Contains functions for score command.
*/


/*! \brief Parses score command-line and performs score. */
int command_score( int argc, int last_arg, char** argv );


/*
 $Log$
 Revision 1.6  2002/10/31 23:14:19  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.5  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.4  2002/07/20 21:34:58  phase1geo
 Separating ability to parse design and score dumpfile.  Now both or either
 can be done (allowing one to parse once and score multiple times).
*/

#endif

