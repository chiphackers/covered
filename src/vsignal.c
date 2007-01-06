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
 \file     vsignal.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "link.h"
#include "obfuscate.h"
#include "sim.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern nibble or_optab[OPTAB_SIZE];
extern char   user_msg[USER_MSG_LENGTH];


/*!
 \param sig    Pointer to vsignal to initialize.
 \param name   Pointer to vsignal name string.
 \param type   Type of signal to create
 \param value  Pointer to vsignal value.
 \param line   Line number that this signal is declared on.
 \param col    Starting column that this signal is declared on.
 
 Initializes the specified vsignal with the values of name, value and lsb.  This
 function is called by the vsignal_create routine and is also useful for
 creating temporary vsignals (reduces the need for dynamic memory allocation).
 for performance enhancing purposes.
*/
void vsignal_init( vsignal* sig, char* name, int type, vector* value, int line, int col ) {

  sig->name            = name;
  sig->pdim_num        = 0;
  sig->udim_num        = 0;
  sig->dim             = NULL;
  sig->suppl.all       = 0;
  sig->suppl.part.type = type;
  sig->suppl.part.col  = col;
  sig->value           = value;
  sig->line            = line;
  sig->exp_head        = NULL;
  sig->exp_tail        = NULL;

}

/*!
 \param name   Full hierarchical name of this vsignal.
 \param type   Type of signal to create
 \param width  Bit width of this vsignal.
 \param line   Line number that this signal is declared on.
 \param col    Starting column that this signal is declared on.

 \returns Pointer to newly created vsignal.

 This function should be called by the Verilog parser or the
 database reading function.  It initializes all of the necessary
 values for a vsignal and returns a pointer to this newly created
 vsignal.
*/
vsignal* vsignal_create( char* name, int type, int width, int line, int col ) {

  vsignal* new_sig;  /* Pointer to newly created vsignal */

  new_sig = (vsignal*)malloc_safe( sizeof( vsignal ), __FILE__, __LINE__ );

  vsignal_init( new_sig, ((name != NULL) ? strdup_safe( name, __FILE__, __LINE__ ) : NULL),
                type, vector_create( width, ((type == SSUPPL_TYPE_MEM) ? VTYPE_MEM : VTYPE_SIG), TRUE ), line, col );

  return( new_sig );

}

/*!
 \param sig  Pointer to signal to create vector for

 Calculates the signal width and creates a vector value that is
 sized to match this width.  This function is called during race condition checking and
 functional unit element sizing function and needs to be called before expression resizing is performed.
*/
void vsignal_create_vec( vsignal* sig ) {

  int     i;    /* Loop iterator */
  vector* vec;  /* Temporary vector used for getting a vector value */

  assert( sig != NULL );
  assert( sig->value != NULL );

  /* If this signal has been previously simulated, don't create a new vector */
  if( !vector_is_set( sig->value ) ) {

    /* Set the initial signal width to 1 */
    sig->value->width = 1;

    /* Calculate the width of the given signal */
    for( i=0; i<(sig->pdim_num + sig->udim_num); i++ ) {
      if( sig->dim[i].msb > sig->dim[i].lsb ) {
        sig->value->width *= ((sig->dim[i].msb - sig->dim[i].lsb) + 1);
      } else {
        sig->value->width *= ((sig->dim[i].lsb - sig->dim[i].msb) + 1);
      }
    }

    /* Set the endianness */
    if( (sig->pdim_num + sig->udim_num) > 0 ) {
      sig->suppl.part.big_endian = (sig->dim[(sig->pdim_num + sig->udim_num)-1].msb < sig->dim[(sig->pdim_num + sig->udim_num)-1].lsb) ? 1 : 0;
    }

    /* Create the vector and assign it to the signal */
    vec = vector_create( sig->value->width, ((sig->suppl.part.type == SSUPPL_TYPE_MEM) ? VTYPE_MEM : VTYPE_SIG), TRUE );
    if( sig->value->value != NULL ) {
      free_safe( sig->value->value );
    }
    sig->value->value = vec->value;
    free_safe( vec );

  }

}

/*!
 \param sig  Pointer to signal to duplicate

 \return Returns a newly allocated and initialized copy of the given signal

 Duplicates the contents of the given signal with the exception of the expression list.
*/
vsignal* vsignal_duplicate( vsignal* sig ) {

  vsignal*  new_sig;  /* Pointer to newly created vsignal */
  exp_link* expl;     /* Pointer to current expression link */
  int       i;        /* Loop iterator */

  assert( sig != NULL );

  new_sig = (vsignal*)malloc_safe( sizeof( vsignal ), __FILE__, __LINE__ );
  new_sig->name      = strdup_safe( sig->name, __FILE__, __LINE__ );
  new_sig->suppl.all = sig->suppl.all;
  new_sig->pdim_num  = sig->pdim_num;
  new_sig->udim_num  = sig->udim_num;
  new_sig->dim       = NULL;
  new_sig->line      = sig->line;
  new_sig->exp_head  = NULL;
  new_sig->exp_tail  = NULL;

  /* Copy the dimension information */
  if( (sig->pdim_num + sig->udim_num) > 0 ) {
    new_sig->dim = (dim_range*)malloc_safe( (sizeof( dim_range ) * (sig->pdim_num + sig->udim_num)), __FILE__, __LINE__ );
    for( i=0; i<(sig->pdim_num + sig->udim_num); i++ ) {
      new_sig->dim[i].msb = sig->dim[i].msb;
      new_sig->dim[i].lsb = sig->dim[i].lsb;
    }
  }

  /* Copy the vector value */
  vector_copy( sig->value, &(new_sig->value) );

  /* Copy the expression pointers */
  expl = sig->exp_head;
  while( expl != NULL ) {
    exp_link_add( expl->exp, &(new_sig->exp_head), &(new_sig->exp_tail) );
    expl = expl->next;
  }

  return( new_sig );

}

/*!
 \param sig      Signal to write to file.
 \param file     Name of file to display vsignal contents to.

 Prints the vsignal information for the specified vsignal to the
 specified file.  This file will be the database coverage file
 for this design.
*/
void vsignal_db_write( vsignal* sig, FILE* file ) {

  int i;  /* Loop iterator */

  /* Don't write this vsignal if it isn't usable by Covered */
  if( (sig->suppl.part.not_handled == 0) &&
      (sig->value->width != -1) &&
      (sig->value->width <= MAX_BIT_WIDTH) &&
      (sig->suppl.part.type != SSUPPL_TYPE_GENVAR) ) {

    /* Display identification and value information first */
    fprintf( file, "%d %s %d %x %d %d",
      DB_TYPE_SIGNAL,
      sig->name,
      sig->line,
      sig->suppl.all,
      sig->pdim_num,
      sig->udim_num
    );

    /* Display dimension information */
    for( i=0; i<(sig->pdim_num + sig->udim_num); i++ ) {
      fprintf( file, " %d %d", sig->dim[i].msb, sig->dim[i].lsb );
    }
    fprintf( file, " " );

    vector_db_write( sig->value, file, ((sig->suppl.part.type == SSUPPL_TYPE_PARAM) || (sig->suppl.part.type == SSUPPL_TYPE_ENUM)) );

    fprintf( file, "\n" );

  }

}

/*!
 \param line        Pointer to current line from database file to parse.
 \param curr_funit  Pointer to current functional unit instantiating this vsignal.

 \return Returns TRUE if vsignal information read successfully; otherwise,
         returns FALSE.

 Creates a new vsignal structure, parses current file line for vsignal
 information and stores it to the specified vsignal.  If there are any problems
 in reading in the current line, returns FALSE; otherwise, returns TRUE.
*/
bool vsignal_db_read( char** line, func_unit* curr_funit ) {

  bool       retval = TRUE;  /* Return value for this function */
  char       name[256];      /* Name of current vsignal */
  vsignal*   sig;            /* Pointer to the newly created vsignal */
  vector*    vec;            /* Vector value for this vsignal */
  int        sline;          /* Declared line number */
  int        pdim_num;       /* Packed dimension number */
  int        udim_num;       /* Unpacked dimension number */
  dim_range* dim    = NULL;  /* Dimensional information */
  ssuppl     suppl;          /* Supplemental field */
  int        chars_read;     /* Number of characters read from line */
  int        i;              /* Loop iterator */

  /* Get name values. */
  if( sscanf( *line, "%s %d %x %d %d%n", name, &sline, &(suppl.all), &pdim_num, &udim_num, &chars_read ) == 5 ) {

    *line = *line + chars_read;

    /* Allocate dimensional information */
    dim = (dim_range*)malloc_safe( (sizeof( dim_range ) * (pdim_num + udim_num)), __FILE__, __LINE__ );

    /* Read in dimensional information */
    i = 0;
    while( (i < (pdim_num + udim_num)) && retval ) {
      if( sscanf( *line, " %d %d%n", &(dim[i].msb), &(dim[i].lsb), &chars_read ) == 2 ) {
        *line = *line + chars_read;
      } else {
        retval = FALSE;
      }
      i++;
    }

    if( !retval ) {

      free_safe( dim );

    } else {

      /* Read in vector information */
      if( vector_db_read( &vec, line ) ) {

        /* Create new vsignal */
        sig = vsignal_create( name, suppl.part.type, vec->width, sline, suppl.part.col );
        sig->suppl.part.assigned   = suppl.part.assigned;
        sig->suppl.part.mba        = suppl.part.mba;
        sig->suppl.part.big_endian = suppl.part.big_endian;
        sig->pdim_num              = pdim_num;
        sig->udim_num              = udim_num;
        sig->dim                   = dim;

        /* Copy over vector value */
        vector_dealloc( sig->value );
        sig->value = vec;

        /* Add vsignal to vsignal list */
        if( curr_funit == NULL ) {
          print_output( "Internal error:  vsignal in database written before its functional unit", FATAL, __FILE__, __LINE__ );
          retval = FALSE;
        } else {
          sig_link_add( sig, &(curr_funit->sig_head), &(curr_funit->sig_tail) );
        }

      } else {

        retval = FALSE;

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Signal to store result of merge into.
 \param line  Pointer to line of CDD file to parse.
 \param same  Specifies if vsignal to merge needs to be exactly the same as the existing vsignal.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Parses specified line for vsignal information and performs merge 
 of the base and in vsignals, placing the resulting merged vsignal 
 into the base vsignal.  If the vsignals are found to be unalike 
 (names are different), an error message is displayed to the user.  
 If both vsignals are the same, perform the merge on the vsignal's 
 vectors.
*/
bool vsignal_db_merge( vsignal* base, char** line, bool same ) {
 
  bool    retval = TRUE;  /* Return value of this function */
  char    name[256];      /* Name of current vsignal */
  int     sline;          /* Declared line number */
  int     pdim_num;       /* Number of packed dimensions */
  int     udim_num;       /* Number of unpacked dimensions */
  int     msb;            /* MSB of current dimension being read */
  int     lsb;            /* LSB of current dimension being read */
  ssuppl  suppl;          /* Supplemental signal information */
  int     chars_read;     /* Number of characters read from line */
  int     i;              /* Loop iterator */

  assert( base != NULL );
  assert( base->name != NULL );

  if( sscanf( *line, "%s %d %x %d %d%n", name, &sline, &(suppl.all), &pdim_num, &udim_num, &chars_read ) == 5 ) {

    *line = *line + chars_read;

    if( !scope_compare( base->name, name ) || (base->pdim_num != pdim_num) || (base->udim_num != udim_num) ) {

      print_output( "Attempting to merge two databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else {

      i = 0;
      while( (i < (pdim_num + udim_num)) && (sscanf( *line, " %d %d%n", &msb, &lsb, &chars_read ) == 2) ) {
        *line = *line + chars_read;
        i++;
      }

      if( i == (pdim_num + udim_num) ) {

        /* Read in vector information */
        retval = vector_db_merge( base->value, line, same );

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param sig       Pointer to signal to propagate change information from.
 \param sim_time  Current simulation time when signal changed.

  When the specified signal in the parameter list has changed values, this function
  is called to propagate the value change to the simulator to cause any statements
  waiting on this value change to be resimulated.
*/
void vsignal_propagate( vsignal* sig, uint64 sim_time ) {

  exp_link* curr_expr;  /* Pointer to current expression in signal list */

  /* Iterate through vsignal's expression list */
  curr_expr = sig->exp_head;
  while( curr_expr != NULL ) {

    /* Add to simulation queue if expression is a RHS, not a function call and not a port assignment */
    if( (ESUPPL_IS_LHS( curr_expr->exp->suppl ) == 0) &&
        (curr_expr->exp->op != EXP_OP_FUNC_CALL) &&
        (curr_expr->exp->op != EXP_OP_PASSIGN) ) {
      sim_expr_changed( curr_expr->exp, sim_time );
    }

    curr_expr = curr_expr->next;

  }

}

/*!
 \param sig       Pointer to vsignal to assign VCD value to.
 \param value     String version of VCD value.
 \param msb       Most significant bit to assign to.
 \param lsb       Least significant bit to assign to.
 \param sim_time  Current simulation time signal is being assigned.

 Assigns the associated value to the specified vsignal's vector.  After this, it
 iterates through its expression list, setting the TRUE and FALSE bits accordingly.
 Finally, calls the simulator expr_changed function for each expression.
*/
void vsignal_vcd_assign( vsignal* sig, char* value, int msb, int lsb, uint64 sim_time ) {

  bool vec_changed;  /* Specifies if assigned value differed from original value */

  assert( sig != NULL );
  assert( sig->value != NULL );

  /*
   Since this signal is coming from the dumpfile, we don't expect to see values for multi-dimensional
   arrays.
  */
  assert( sig->udim_num == 0 );

  /*
   VCS seems to create funny MSB values for packed arrays, so if the pdim_num is more than 1, adjust
   the MSB accordingly.
  */
  if( (sig->pdim_num > 1) && (msb >= sig->value->width) ) {
    msb = sig->value->width - 1;
  }

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "Assigning vsignal %s[%d:%d] (lsb=%d) to value %s",
            obf_sig( sig->name ), msb, lsb, sig->dim[0].lsb, value );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Set vsignal value to specified value */
  if( lsb > 0 ) {
    vec_changed = vector_vcd_assign( sig->value, value, (msb - sig->dim[0].lsb), (lsb - sig->dim[0].lsb) );
  } else {
    vec_changed = vector_vcd_assign( sig->value, value, msb, lsb );
  }

  /* Don't go through the hassle of updating expressions if value hasn't changed */
  if( vec_changed ) {

    /* Propagate signal changes to rest of design */
    vsignal_propagate( sig, sim_time );

  } 

}

/*!
 \param sig   Pointer to vsignal to add expression to.
 \param expr  Expression to add to list.

 Adds the specified expression to the end of this vsignal's expression
 list.
*/
void vsignal_add_expression( vsignal* sig, expression* expr ) {

  exp_link_add( expr, &(sig->exp_head), &(sig->exp_tail) );

}

/*!
 \param sig  Pointer to vsignal to display to standard output.

 Displays vsignal's name, dimensional info, width and value vector to the standard output.
*/
void vsignal_display( vsignal* sig ) {

  int i;  /* Loop iterator */

  assert( sig != NULL );

  printf( "  Signal =>  name: %s, ", obf_sig( sig->name ) );

  if( sig->pdim_num > 0 ) {
    printf( "packed: " );
    for( i=sig->udim_num; i<(sig->pdim_num + sig->udim_num); i++ ) {
      printf( "[%d:%d]", sig->dim[i].msb, sig->dim[i].lsb );
    }
    printf( ", " );
  }

  if( sig->udim_num > 0 ) {
    printf( "unpacked: " );
    for( i=0; i<sig->udim_num; i++ ) {
      printf( "[%d:%d]", sig->dim[i].msb, sig->dim[i].lsb );
    }
    printf( ", " );
  }

  vector_display_value( sig->value->value, sig->value->width );
  printf( "\n" );

}

/*!
 \param str  String version of vsignal.

 \return Returns pointer to newly created vsignal structure, or returns
         NULL is specified string does not properly describe a vsignal.

 Converts the specified string describing a Verilog design vsignal.  The
 vsignal may be a standard vsignal name, a single bit select vsignal or a
 multi-bit select vsignal.
*/
vsignal* vsignal_from_string( char** str ) {

  vsignal* sig;             /* Pointer to newly created vsignal */
  char     name[4096];      /* Signal name */
  int      left;            /* Left selection value of the signal */
  int      right;           /* Right selection value of the signal */
  int      width;           /* Width of the signal */
  int      lsb;             /* LSB of the signal */
  int      big_endian = 0;  /* Endianness of the signal */
  int      chars_read;      /* Number of characters read from string */

  if( sscanf( *str, "%[a-zA-Z0-9_]\[%d:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    if( right > left ) {
      width      = (right - left) + 1;
      lsb        = left;
      big_endian = 1;
    } else {
      width      = (left - right) + 1;
      lsb        = right;
      big_endian = 0;
    }
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, width, 0, 0 );
    sig->pdim_num   = 1;
    sig->dim        = (dim_range*)malloc_safe( (sizeof( dim_range ) * 1), __FILE__, __LINE__ );
    sig->dim[0].msb = left;
    sig->dim[0].lsb = right;
    sig->suppl.part.big_endian = big_endian;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d+:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT_POS, right, 0, 0 );
    sig->pdim_num   = 1;
    sig->dim        = (dim_range*)malloc_safe( (sizeof( dim_range ) * 1), __FILE__, __LINE__ );
    sig->dim[0].msb = left + right;
    sig->dim[0].lsb = left;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d-:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT_NEG, right, 0, 0 );
    sig->pdim_num   = 1;
    sig->dim        = (dim_range*)malloc_safe( (sizeof( dim_range ) * 1), __FILE__, __LINE__ );
    sig->dim[0].msb = left - right;
    sig->dim[0].lsb = left;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d]%n", name, &right, &chars_read ) == 2 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, 0, 0 );
    sig->pdim_num   = 1;
    sig->dim        = (dim_range*)malloc_safe( (sizeof( dim_range ) * 1), __FILE__, __LINE__ );
    sig->dim[0].msb = right;
    sig->dim[0].lsb = right;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]%n", name, &chars_read ) == 1 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, 0, 0 );
    /* Specify that this width is unknown */
    sig->value->width = 0;
    *str += chars_read;
  } else {
    sig = NULL;
  }

  return( sig );

}

/*!
 \param expr  Pointer to expression to get width for
 \param sig   Pointer to signal to get width for

 \return Returns width of the given expression that is bound to the given signal.
*/
int vsignal_calc_width_for_expr( expression* expr, vsignal* sig ) {

  int exp_dim;    /* Expression dimension number */
  int width = 1;  /* Return value for this function */
  int i;          /* Loop iterator */

  assert( expr != NULL );
  assert( sig != NULL );

  /* Get expression dimension value */
  exp_dim = expression_get_curr_dimension( expr );

  /* Calculate width */
  for( i=(exp_dim + 1); i < (sig->pdim_num + sig->udim_num); i++ ) {
    if( sig->dim[i].msb > sig->dim[i].lsb ) {
      width *= (sig->dim[i].msb - sig->dim[i].lsb) + 1;
    } else {
      width *= (sig->dim[i].lsb - sig->dim[i].msb) + 1;
    }
  }

  return( width );

}

/*!
 \param expr     Pointer to expression to get LSB for
 \param sig      Pointer to signal to get LSB for
 \param lsb_val  Calculated LSB value from this expression

 \return Returns the LSB of the given signal for the given expression.
*/
int vsignal_calc_lsb_for_expr( expression* expr, vsignal* sig, int lsb_val ) {

  return( vsignal_calc_width_for_expr( expr, sig ) * lsb_val );

}

/*!
 \param sig  Pointer to vsignal to deallocate.

 Deallocates all malloc'ed memory back to the heap for the specified
 vsignal.
*/
void vsignal_dealloc( vsignal* sig ) {

  exp_link* curr_expl;  /* Pointer to current expression link to set to NULL */

  if( sig != NULL ) {

    /* Free the signal name */
    free_safe( sig->name );
    sig->name = NULL;

    /* Free the dimension information */
    free_safe( sig->dim );

    /* Free up memory for value */
    vector_dealloc( sig->value );
    sig->value = NULL;

    /* Free up memory for expression list */
    curr_expl = sig->exp_head;
    while( curr_expl != NULL ) {
      curr_expl->exp->sig = NULL;
      curr_expl = curr_expl->next;
    }

    exp_link_delete_list( sig->exp_head, FALSE );
    sig->exp_head = NULL;

    /* Finally free up the memory for this vsignal */
    free_safe( sig );

  }

}

/*
 $Log$
 Revision 1.40.2.1  2007/01/05 15:55:39  phase1geo
 Fixing bug 1628356 and fixing problem with VCS generated VCD file parsing
 for multi-dimensional packed arrays.

 Revision 1.40  2006/11/27 04:11:42  phase1geo
 Adding more changes to properly support thread time.  This is a work in progress
 and regression is currently broken for the moment.  Checkpointing.

 Revision 1.39  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.38  2006/09/25 22:22:29  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.37  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.36  2006/09/21 22:44:20  phase1geo
 More updates to regressions for latest changes to support memories/multi-dimensional
 arrays.  We still have a handful of VCS diagnostics that are failing.  Checkpointing
 for now.

 Revision 1.35  2006/09/21 04:20:59  phase1geo
 Fixing endianness diagnostics.  Still getting memory error with some diagnostics
 in regressions (ovl1 is one of them).  Updated regression.

 Revision 1.34  2006/09/20 22:38:10  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.33  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.32  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.31  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

 Revision 1.30  2006/08/18 22:07:46  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.29  2006/08/11 15:16:49  phase1geo
 Joining slist3.3 diagnostic to latest development branch.  Adding changes to
 fix memory issues from bug 1535412.

 Revision 1.28  2006/07/27 02:04:30  phase1geo
 Fixing problem with parameter usage in a generate block for signal sizing.

 Revision 1.27  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.26  2006/07/11 04:59:08  phase1geo
 Reworking the way that instances are being generated.  This is to fix a bug and
 pave the way for generate loops for instances.  Code not working at this point
 and may cause serious problems for regression runs.

 Revision 1.25  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.24  2006/05/25 12:11:02  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.23  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.22.4.1.4.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.22.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.22  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.21  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.20  2006/02/01 15:13:11  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.19  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.18  2006/01/23 17:23:28  phase1geo
 Fixing scope issues that came up when port assignment was added.  Full regression
 now passes.

 Revision 1.17  2006/01/23 03:53:30  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.16  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.15  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.14  2005/12/01 18:35:17  phase1geo
 Fixing bug where functions in continuous assignments could cause the
 assignment to constantly be reevaluated (infinite looping).  Added new nested_block2
 diagnostic to verify nested named blocks in functions.  Also verifies that nested
 named blocks can call functions in the same module.  Also modified NB_CALL expressions
 to act like functions (no context switching involved).  Full regression passes.

 Revision 1.13  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.12  2005/11/21 22:21:58  phase1geo
 More regression updates.  Also made some updates to debugging output.

 Revision 1.11  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.10  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.9  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.8  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.7  2005/02/16 13:45:04  phase1geo
 Adding value propagation function to vsignal.c and adding this propagation to
 BASSIGN expression assignment after the assignment occurs.

 Revision 1.6  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.5  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.4  2004/11/07 05:51:24  phase1geo
 If assigned signal value did not change, do not cause associated expression tree(s)
 to be re-evaluated.

 Revision 1.3  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.2  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.1  2004/03/30 15:42:15  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.48  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.47  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.46  2003/11/29 06:55:49  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.45  2003/11/12 17:34:03  phase1geo
 Fixing bug where signals are longer than allowable bit width.

 Revision 1.44  2003/11/05 05:22:56  phase1geo
 Final fix for bug 835366.  Full regression passes once again.

 Revision 1.43  2003/10/20 16:05:06  phase1geo
 Fixing bug for older versions of Icarus that does not output the correct
 bit select in the VCD dumpfile.  Covered will automatically adjust the bit-select
 range if this occurrence is found in the dumpfile.

 Revision 1.42  2003/10/19 04:23:49  phase1geo
 Fixing bug in VCD parser for new Icarus Verilog VCD dumpfile formatting.
 Fixing bug in signal.c for signal merging.  Updates all CDD files to match
 new format.  Added new diagnostics to test advanced FSM state variable
 features.

 Revision 1.41  2003/10/17 21:59:07  phase1geo
 Fixing signal_vcd_assign function to properly adjust msb and lsb based on
 the lsb of the signal that is being assigned to.

 Revision 1.40  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.39  2003/10/16 04:26:01  phase1geo
 Adding new fsm5 diagnostic to testsuite and regression.  Added proper support
 for FSM variables that are not able to be bound correctly.  Fixing bug in
 signal_from_string function.

 Revision 1.38  2003/10/13 22:10:07  phase1geo
 More changes for FSM support.  Still not quite there.

 Revision 1.37  2003/10/11 05:15:08  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.36  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.35  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.

 Revision 1.34  2003/09/22 03:46:24  phase1geo
 Adding support for single state variable FSMs.  Allow two different ways to
 specify FSMs on command-line.  Added diagnostics to verify new functionality.

 Revision 1.33  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.32  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.31  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.30  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.29  2003/02/26 23:00:50  phase1geo
 Fixing bug with single-bit parameter handling (param4.v diagnostic miscompare
 between Linux and Irix OS's).  Updates to testsuite and new diagnostic added
 for additional testing in this area.

 Revision 1.28  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.27  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.26  2002/12/29 06:09:32  phase1geo
 Fixing bug where output was not squelched in report command when -Q option
 is specified.  Fixed bug in preprocessor where spaces where added in when newlines
 found in C-style comment blocks.  Modified regression run to check CDD file and
 generated module and instance reports.  Started to add code to handle signals that
 are specified in design but unused in Covered.

 Revision 1.25  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.24  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.23  2002/10/31 23:14:23  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.22  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.21  2002/10/24 05:48:58  phase1geo
 Additional fixes for MBIT_SEL.  Changed some philosophical stuff around for
 cleaner code and for correctness.  Added some development documentation for
 expressions and vectors.  At this point, there is a bug in the way that
 parameters are handled as far as scoring purposes are concerned but we no
 longer segfault.

 Revision 1.20  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.19  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.18  2002/10/01 13:21:25  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.17  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.16  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.15  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.14  2002/08/14 04:52:48  phase1geo
 Removing unnecessary calls to signal_dealloc function and fixing bug
 with signal_dealloc function.

 Revision 1.13  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.12  2002/07/19 13:10:07  phase1geo
 Various fixes to binding scheme.

 Revision 1.11  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.10  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.9  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.8  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.7  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.6  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

