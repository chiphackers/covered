#ifndef __EXCLUDE_H__
#define __EXCLUDE_H__

/*!
 \file     exclude.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/22/2006
 \brief    Contains functions for handling user-specified exclusion of coverage results.
*/

#include "defines.h"


/*! \brief Sets the exclude bit of the specified expression and recalculates coverage */
void exclude_expr_assign_and_recalc( expression* expr, funit_inst* inst, bool excluded );

/*! \brief Sets the exclude bit of the specified signal and recalculates coverage */
void exclude_sig_assign_and_recalc( signal* sig, funit_inst* inst, bool excluded );

/*
 $Log$
*/

#endif

