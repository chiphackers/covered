#ifndef __DB_H__
#define __DB_H__

/*!
 \file     db.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/7/2001
 \brief    Contains functions for writing and reading contents of
           covered database file.
*/


//! Writes contents of expressions, modules and signals to database file.
bool db_write( char* file );

//! Reads contents of database file and stores into internal lists.
bool db_read( char* file, int read_mode );

//! Adds specified module node to module tree.  Called by parser.
void db_add_instance( char* scope, char* modname );

//! Adds specified module to module list.  Called by parser.
void db_add_module( char* name, char* file );

//! Adds specified signal to signal list.  Called by parser.
void db_add_signal( char* name, int width, int lsb, int is_static );

//! Finds specified signal in module and returns pointer to the signal structure.  Called by parser.
signal* db_find_signal( char* name );

//! Creates new expression from specified information.  Called by parser and db_add_expression.
expression* db_create_expression( expression* right, expression* left, int op, int line, char* sig_name );

//! Adds specified expression to expression list.  Called by parser.
void db_add_expression( expression* root );

//! Sets current VCD scope to specified scope.
void db_set_vcd_scope( char* scope );

//! Adds symbol to signal specified by name.
void db_assign_symbol( char* name, char* symbol );

//! Finds specified signal and returns TRUE if found.
bool db_symbol_found( char* symbol );

//! Finds specified signal, sets the value and adds its expression to expression queue. 
void db_find_set_add_signal( char* symbol, vector* vec );

//! Returns width of signal for specified symbol.
int db_get_signal_size( char* symbol );

//! Performs a timestep for all signal changes during this timestep.
void db_do_timestep( int time ); 

#endif

