/*!
 \file     signal.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "signal.h"
#include "expr.h"
#include "link.h"
#include "vector.h"
#include "module.h"
#include "util.h"


/*!
 \param sig        Pointer to signal to initialize.
 \param name       Pointer to signal name string.
 \param value      Pointer to signal value.
 
 Initializes the specified signal with the values of name and value.  This
 function is called by the signal_create routine and is also useful for
 creating temporary signals (reduces the need for dynamic memory allocation).
 for performance enhancing purposes.
*/
void signal_init( signal* sig, char* name, vector* value ) {

  sig->name     = name;
  sig->value    = value;
  sig->exp_head = NULL;
  sig->exp_tail = NULL;

}

/*!
 \param name       Full hierarchical name of this signal.
 \param width      Bit width of this signal.
 \param lsb        Bit position of least significant bit.
 \param is_static  Specifies if this signal is a static value.

 \returns Pointer to newly created signal.

 This function should be called by the Verilog parser or the
 database reading function.  It initializes all of the necessary
 values for a signal and returns a pointer to this newly created
 signal.
*/
signal* signal_create( char* name, int width, int lsb, int is_static ) {

  signal* new_sig;     /* Pointer to newly created signal */

  new_sig = (signal*)malloc_safe( sizeof( signal ) );

  signal_init( new_sig, strdup( name ), vector_create( width, lsb ) );

  return( new_sig );

}

/*!
 \param base  Signal to store result of merge into.
 \param in    Signal to be merged into base signal.

 Performs merge of the base and in signals, placing the resulting
 merged signal into the base signal.  If the signals are found to
 be unalike (names are different), an error message is displayed
 to the user.  If both signals are the same, perform the merge on
 the signal's vectors.
*/
void signal_merge( signal* base, signal* in ) {

  assert( base != NULL );
  assert( base->name != NULL );

  assert( in != NULL );
  assert( in->name != NULL );

  if( strcmp( base->name, in->name ) != 0 ) {

    print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
    exit( 1 );

  } else {

    /* Perform merge on vectors */
    vector_merge( base->value, in->value );

  }

}

/*!
 \param sig      Signal to write to file.
 \param file     Name of file to display signal contents to.
 \param modname  Name of module that this signal belongs to.

 Prints the signal information for the specified signal to the
 specified file.  This file will be the database coverage file
 for this design.
*/
void signal_db_write( signal* sig, FILE* file, char* modname ) {

  exp_link* curr;      /* Pointer to current expression link element */

  /* Display identification and value information first */
  fprintf( file, "%d %s %s ",
    DB_TYPE_SIGNAL,
    sig->name,
    modname
  );

  vector_db_write( sig->value, file, FALSE );

  curr = sig->exp_head;
  while( curr != NULL ) {
    fprintf( file, " %d", expression_get_id( curr->exp ) );
    curr = curr->next;
  }

  fprintf( file, "\n" );

}

/*!
 \param line      Pointer to current line from database file to parse.
 \param curr_mod  Pointer to current module instantiating this signal.

 \return Returns TRUE if signal information read successfully; otherwise,
         returns FALSE.

 \bug 
 A signal will only look in the current module for a matching expression.  In the case
 of a hierarchical reference, it is possible that an expression outside the current module
 is referencing this signal.  We need to check for this case (hierarchical expression)
 and find the expression elsewhere.

 Creates a new signal structure, parses current file line for signal
 information and stores it to the specified signal.  If there are any problems
 in reading in the current line, returns FALSE; otherwise, returns TRUE.
*/
bool signal_db_read( char** line, module* curr_mod ) {

  bool        retval = TRUE;   /* Return value for this function                   */
  char        name[256];       /* Name of current signal                           */
  signal*     sig;             /* Pointer to the newly created signal              */
  vector*     vec;             /* Vector value for this signal                     */
  int         exp_id;          /* Expression ID                                    */
  char        tmp[2];          /* Temporary holder for semicolon                   */
  int         chars_read;      /* Number of characters read from line              */
  int         i;               /* Loop iterator                                    */
  char        modname[4096];   /* Name of signal's module                          */
  expression  texp;            /* Temporary expression link for searching purposes */
  exp_link*   expl;            /* Temporary expression link for storage            */
  char        msg[4096];       /* Error message string                             */
  expression* curr_parent;     /* Pointer to current parent being traversed        */

  /* Get name values. */
  if( sscanf( *line, "%s %s %n", name, modname, &chars_read ) == 2 ) {

    *line = *line + chars_read;

    /* Read in vector information */
    if( vector_db_read( &vec, line ) ) {

      /* Create new signal */
      sig = signal_create( name, vec->width, vec->lsb, 0 );

      /* Copy over vector value */
      for( i=0; i<vec->width; i++ ) {
	sig->value->value = vec->value;
      }

      /* Add signal to signal list */
      if( (curr_mod == NULL) || (strcmp( curr_mod->name, modname ) != 0) ) {
        print_output( "Internal error:  signal in database written before its module", FATAL );
        retval = FALSE;
      } else {
        sig_link_add( sig, &(curr_mod->sig_head), &(curr_mod->sig_tail) );
      }

      /* Read in expressions */
      while( sscanf( *line, "%d%n", &exp_id, &chars_read ) == 1 ) {

        *line = *line + chars_read;

        /* Find expression in current module and add it to signal list */
	texp.id = exp_id;

        if( (expl = exp_link_find( &texp, curr_mod->exp_head )) != NULL ) {

	  exp_link_add( expl->exp, &(sig->exp_head), &(sig->exp_tail) );

	  /* 
	   If expression is a signal holder, we need to set the expression's vector to point
	   to our vector and set its signal pointer to point to us.
	  */
          if( (SUPPL_OP( expl->exp->suppl ) == EXP_OP_SIG      ) ||
              (SUPPL_OP( expl->exp->suppl ) == EXP_OP_SBIT_SEL ) ||
              (SUPPL_OP( expl->exp->suppl ) == EXP_OP_MBIT_SEL ) ) {

            if( SUPPL_OP( expl->exp->suppl ) == EXP_OP_SIG ) {
              expl->exp->value = sig->value;
              expl->exp->sig   = sig;
            } else if( (SUPPL_OP( expl->exp->suppl ) == EXP_OP_SBIT_SEL) || (SUPPL_OP( expl->exp->suppl ) == EXP_OP_MBIT_SEL) ) {
              expl->exp->value->value = sig->value->value;
              expl->exp->sig          = sig;
            }

            /* Traverse parent links, setting its width if it is set to 0. */
            curr_parent = expl->exp->parent;
            while( (curr_parent != NULL) && (curr_parent->value->width == 0) ) {
              expression_create_value( curr_parent, sig->value->width, sig->value->lsb );
              curr_parent = curr_parent->parent;
            }

          }

        } else {

          snprintf( msg, 4096, "Expression %d not found for signal %s", texp.id, sig->name );
	  print_output( msg, FATAL );
	  retval = FALSE;
          exit( 1 );

        }

      }

    } else {

      retval = FALSE;

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param sig       Pointer to signal to set value to.
 \param value     Value to set signal value to.
 \param num_bits  Number of bits wide that value is.
 \param from_lsb  Least significant bit of assign value.
 \param to_lsb    Least significant bit of signal being assigned.
 \return Returns TRUE if value assignment is successful; otherwise, returns
         FALSE.

 This function is called by the VCD reading function when the matching
 signal is found.  If the signal has never been set to a value before
 (value is NULL), copy the contents of the value verbatim.  If the signal
 value has been set previously, compare the old value to the new value
 bit for bit, setting the toggle bits accordingly.  After this is achieved,
 copy the new value to the signal value.  If the specified value width,
 does not match this signal's width, we have an internal error (VCD file
 does not match the design read in), so we return FALSE.  Otherwise, return
 with a value of TRUE.
*/
bool signal_set_value( signal* sig, nibble* value, int num_bits, int from_lsb, int to_lsb ) {

  return( vector_set_value( sig->value, value, num_bits, from_lsb, to_lsb ) );

}

/*!
 \param sig   Pointer to signal to add expression to.
 \param expr  Expression to add to list.

 Adds the specified expression to the end of this signal's expression
 list.
*/
void signal_add_expression( signal* sig, expression* expr ) {

  exp_link_add( expr, &(sig->exp_head), &(sig->exp_tail) );

}

/*!
 \param sig  Pointer to signal to display to standard output.

 Displays signal's name, width, lsb and value vector to the standard output.
*/
void signal_display( signal* sig ) {

  assert( sig != NULL );

  printf( "  Signal =>  name: %s, ", sig->name );
  
  vector_display( sig->value );

}

/*!
 \param sig  Pointer to signal to deallocate.

 Deallocates all malloc'ed memory back to the heap for the specified
 signal.
*/
void signal_dealloc( signal* sig ) {

  if( sig != NULL ) {

    if( sig->name != NULL ) {
      free_safe( sig->name );
      sig->name = NULL;
    }

    /* Free up memory for value */
    vector_dealloc( sig->value );
    sig->value = NULL;

    /* Free up memory for expression list */
    exp_link_delete_list( sig->exp_head, FALSE );
    sig->exp_head = NULL;

    /* Finally free up the memory for this signal */
    free_safe( sig );

  }

}

