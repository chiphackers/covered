#ifndef __SIG_DEP_H__
#define __SIG_DEP_H__

/*!
 \file    sig_dep.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/5/2003
 \brief    Contains functions for handling signal dependencies.
*/

#include "defines.h"

/*! \brief Creates array of signals from specified statement tree. */
void sig_dep_create( signal* sig, statement* root, statement* curr );

/*
 $Log$
*/

#endif

