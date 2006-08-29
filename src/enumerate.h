#ifndef __ENUMERATE_H__
#define __ENUMERATE_H__

/*!
 \file     enumerate.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/29/2006
 \brief    Contains functions for handling enumerations.
*/

#include "defines.h"


/*! \brief Allocates, initializes and adds a new enumerated item to the given functional unit */
void enumerate_add_item( vsignal* enum_sig, static_expr* value, func_unit* funit );

/*! \brief Sets the last status of the last item in the enumerated list */
void enumerate_end_list( func_unit* funit );

/*! \brief Resolves all enumerations within the given functional unit instance */
void enumerate_resolve( funit_inst* inst );

/*! \brief Deallocates enumeration list from given functional unit */
void enumerate_dealloc_list( func_unit* funit );


/*
 $Log$
*/

#endif

