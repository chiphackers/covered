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
 \file     vsignal.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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


extern char   user_msg[USER_MSG_LENGTH];
extern bool   debug_mode;
extern isuppl info_suppl;


/*!
 Initializes the specified vsignal with the values of name, value and lsb.  This
 function is called by the vsignal_create routine and is also useful for
 creating temporary vsignals (reduces the need for dynamic memory allocation).
 for performance enhancing purposes.
*/
static void vsignal_init(
  vsignal*     sig,    /*!< Pointer to vsignal to initialize */
  char*        name,   /*!< Pointer to vsignal name string */
  unsigned int type,   /*!< Type of signal to create */
  vector*      value,  /*!< Pointer to vsignal value */
  unsigned int line,   /*!< Line number that this signal is declared on */
  unsigned int col     /*!< Starting column that this signal is declared on */
) { PROFILE(VSIGNAL_INIT);

  sig->id              = 0;
  sig->name            = name;
  sig->pdim_num        = 0;
  sig->udim_num        = 0;
  sig->dim             = NULL;
  sig->suppl.all       = 0;
  sig->suppl.part.type = type;
  sig->suppl.part.col  = col;
  sig->value           = value;
  sig->line            = line;
  sig->exps            = NULL;
  sig->exp_size        = 0;

  PROFILE_END;

}

/*!
 \returns Pointer to newly created vsignal.

 This function should be called by the Verilog parser or the
 database reading function.  It initializes all of the necessary
 values for a vsignal and returns a pointer to this newly created
 vsignal.
*/
vsignal* vsignal_create(
  const char*  name,   /*!< Full hierarchical name of this vsignal */
  unsigned int type,   /*!< Type of signal to create */
  unsigned int width,  /*!< Bit width of this vsignal */
  unsigned int line,   /*!< Line number that this signal is declared on */
  unsigned int col     /*!< Starting column that this signal is declared on */
) { PROFILE(VSIGNAL_CREATE);

  vsignal*     new_sig;  /* Pointer to newly created vsignal */
  unsigned int vtype;

  new_sig = (vsignal*)malloc_safe( sizeof( vsignal ) );

  /* Calculate the type */
  switch( type ) {
    case SSUPPL_TYPE_DECL_REAL      :
    case SSUPPL_TYPE_PARAM_REAL     :
    case SSUPPL_TYPE_IMPLICIT_REAL  :  vtype = VDATA_R64;  break;
    case SSUPPL_TYPE_DECL_SREAL     :
    case SSUPPL_TYPE_IMPLICIT_SREAL :  vtype = VDATA_R32;  break;
    default                         :  vtype = VDATA_UL;   break;
  }

  vsignal_init( new_sig, ((name != NULL) ? strdup_safe( name ) : NULL),
                type, vector_create( width, ((type == SSUPPL_TYPE_MEM) ? VTYPE_MEM : VTYPE_SIG), vtype, TRUE ), line, col );

  PROFILE_END;

  return( new_sig );

}

#ifndef RUNLIB
/*!
 \throws anonymous expression_set_value

 Calculates the signal width and creates a vector value that is
 sized to match this width.  This function is called during race condition checking and
 functional unit element sizing function and needs to be called before expression resizing is performed.
*/
void vsignal_create_vec(
  vsignal* sig  /*!< Pointer to signal to create vector for */
) { PROFILE(VSIGNAL_CREATE_VEC);

  unsigned int i;     /* Loop iterator */
  vector*      vec;   /* Temporary vector used for getting a vector value */

  assert( sig != NULL );
  assert( sig->value != NULL );

  /* If this signal has been previously simulated, don't create a new vector */
  if( !sig->value->suppl.part.set ) {

    unsigned int vtype;

    /* Deallocate the old memory */
    vector_dealloc_value( sig->value );

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

    /* Figure out the vector data type to create */
    switch( sig->suppl.part.type ) {
      case SSUPPL_TYPE_DECL_REAL      :
      case SSUPPL_TYPE_PARAM_REAL     :
      case SSUPPL_TYPE_IMPLICIT_REAL  :  vtype = VDATA_R64;  break;
      case SSUPPL_TYPE_DECL_SREAL     :
      case SSUPPL_TYPE_IMPLICIT_SREAL :  vtype = VDATA_R32;  break;
      default                         :  vtype = VDATA_UL;   break;
    }

    /* Create the vector and assign it to the signal */
    vec = vector_create( sig->value->width, ((sig->suppl.part.type == SSUPPL_TYPE_MEM) ? VTYPE_MEM : VTYPE_SIG), vtype, TRUE );
    sig->value->value.ul = vec->value.ul;
    free_safe( vec, sizeof( vector ) );

    /* Iterate through expression list, setting the expression to this signal */
    for( i=0; i<sig->exp_size; i++ ) {
      if( (sig->exps[i]->op != EXP_OP_FUNC_CALL) && (sig->exps[i]->op != EXP_OP_PASSIGN) ) {
        expression_set_value( sig->exps[i], sig, NULL );
      }
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a newly allocated and initialized copy of the given signal

 Duplicates the contents of the given signal with the exception of the expression list.
*/
vsignal* vsignal_duplicate(
  vsignal* sig  /*!< Pointer to signal to duplicate */
) { PROFILE(VSIGNAL_DUPLICATE);

  vsignal*     new_sig;  /* Pointer to newly created vsignal */
  unsigned int i;        /* Loop iterator */

  assert( sig != NULL );

  new_sig = (vsignal*)malloc_safe( sizeof( vsignal ) );
  new_sig->name      = strdup_safe( sig->name );
  new_sig->suppl.all = sig->suppl.all;
  new_sig->pdim_num  = sig->pdim_num;
  new_sig->udim_num  = sig->udim_num;
  new_sig->dim       = NULL;
  new_sig->line      = sig->line;
  new_sig->exps      = NULL;
  new_sig->exp_size  = 0;

  /* Copy the dimension information */
  if( (sig->pdim_num + sig->udim_num) > 0 ) {
    new_sig->dim = (dim_range*)malloc_safe( sizeof( dim_range ) * (sig->pdim_num + sig->udim_num) );
    for( i=0; i<(sig->pdim_num + sig->udim_num); i++ ) {
      new_sig->dim[i].msb = sig->dim[i].msb;
      new_sig->dim[i].lsb = sig->dim[i].lsb;
    }
  }

  /* Copy the vector value */
  vector_clone( sig->value, &(new_sig->value) );

  /* Copy the expression pointers */
  for( i=0; i<sig->exp_size; i++ ) {
    exp_link_add( sig->exps[i], &(new_sig->exps), &(new_sig->exp_size) );
  }

  PROFILE_END;

  return( new_sig );

}
#endif /* RUNLIB */

/*!
 Prints the vsignal information for the specified vsignal to the
 specified file.  This file will be the database coverage file
 for this design.
*/
void vsignal_db_write(
  vsignal* sig,  /*!< Signal to write to file */
  FILE*    file  /*!< Name of file to display vsignal contents to */
) { PROFILE(VSIGNAL_DB_WRITE);

  unsigned int i;  /* Loop iterator */

  /* Don't write this vsignal if it isn't usable by Covered */
  if( (sig->suppl.part.not_handled == 0) &&
      (sig->value->width != 0) &&
      (sig->value->width <= MAX_BIT_WIDTH) &&
      (sig->suppl.part.type != SSUPPL_TYPE_GENVAR) ) {

    /* Display identification and value information first */
    fprintf( file, "%d %s %d %d %x %u %u",
      DB_TYPE_SIGNAL,
      sig->name,
      sig->id,
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

    vector_db_write( sig->value, file, ((sig->suppl.part.type == SSUPPL_TYPE_PARAM) || (sig->suppl.part.type == SSUPPL_TYPE_PARAM_REAL) || (sig->suppl.part.type == SSUPPL_TYPE_ENUM)), SIGNAL_IS_NET( sig ) );

    fprintf( file, "\n" );

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw vector_db_read

 Creates a new vsignal structure, parses current file line for vsignal
 information and stores it to the specified vsignal.  If there are any problems
 in reading in the current line, returns FALSE; otherwise, returns TRUE.
*/
void vsignal_db_read(
  char**     line,       /*!< Pointer to current line from database file to parse */
  func_unit* curr_funit  /*!< Pointer to current functional unit instantiating this vsignal */
) { PROFILE(VSIGNAL_DB_READ);

  char         name[256];      /* Name of current vsignal */
  vsignal*     sig;            /* Pointer to the newly created vsignal */
  vector*      vec;            /* Vector value for this vsignal */
  int          id;             /* Signal ID */
  int          sline;          /* Declared line number */
  unsigned int pdim_num;       /* Packed dimension number */
  unsigned int udim_num;       /* Unpacked dimension number */
  dim_range*   dim    = NULL;  /* Dimensional information */
  ssuppl       suppl;          /* Supplemental field */
  int          chars_read;     /* Number of characters read from line */
  unsigned int i;              /* Loop iterator */

  /* Get name values. */
  if( sscanf( *line, "%s %d %d %x %u %u%n", name, &id, &sline, &(suppl.all), &pdim_num, &udim_num, &chars_read ) == 6 ) {

    *line = *line + chars_read;

    /* Allocate dimensional information */
    dim = (dim_range*)malloc_safe( sizeof( dim_range ) * (pdim_num + udim_num) );

    Try {

      /* Read in dimensional information */
      i = 0;
      while( i < (pdim_num + udim_num) ) {
        if( sscanf( *line, " %d %d%n", &(dim[i].msb), &(dim[i].lsb), &chars_read ) == 2 ) {
          *line = *line + chars_read;
        } else {
          print_output( "Unable to parse signal line in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
        i++;
      }

      /* Read in vector information */
      vector_db_read( &vec, line );

    } Catch_anonymous {
      free_safe( dim, sizeof( dim_range ) );
      Throw 0;
    }

    /* Create new vsignal */
    sig = vsignal_create( name, suppl.part.type, vec->width, sline, suppl.part.col );
    sig->id                    = id;
    sig->suppl.part.assigned   = suppl.part.assigned;
    sig->suppl.part.mba        = suppl.part.mba;
    sig->suppl.part.big_endian = suppl.part.big_endian;
    sig->suppl.part.excluded   = suppl.part.excluded;
    sig->pdim_num              = pdim_num;
    sig->udim_num              = udim_num;
    sig->dim                   = dim;

    /* Copy over vector value */
    vector_dealloc( sig->value );
    sig->value = vec;

    /* Add vsignal to vsignal list */
    if( curr_funit == NULL ) {
      print_output( "Internal error:  vsignal in database written before its functional unit", FATAL, __FILE__, __LINE__ );
      Throw 0;
    } else {
      sig_link_add( sig, TRUE, &(curr_funit->sigs), &(curr_funit->sig_size), &(curr_funit->sig_no_rm_index) );
    }

  } else {

    print_output( "Unable to parse signal line in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 \throws anonymous vector_db_merge Throw Throw

 Parses specified line for vsignal information and performs merge 
 of the base and in vsignals, placing the resulting merged vsignal 
 into the base vsignal.  If the vsignals are found to be unalike 
 (names are different), an error message is displayed to the user.  
 If both vsignals are the same, perform the merge on the vsignal's 
 vectors.
*/
void vsignal_db_merge(
  vsignal* base,  /*!< Signal to store result of merge into */
  char**   line,  /*!< Pointer to line of CDD file to parse */
  bool     same   /*!< Specifies if vsignal to merge needs to be exactly the same as the existing vsignal */
) { PROFILE(VSIGNAL_DB_MERGE);
 
  char         name[256];   /* Name of current vsignal */
  int          id;          /* Unique ID of current signal */
  int          sline;       /* Declared line number */
  unsigned int pdim_num;    /* Number of packed dimensions */
  unsigned int udim_num;    /* Number of unpacked dimensions */
  int          msb;         /* MSB of current dimension being read */
  int          lsb;         /* LSB of current dimension being read */
  ssuppl       suppl;       /* Supplemental signal information */
  int          chars_read;  /* Number of characters read from line */
  unsigned int i;           /* Loop iterator */

  assert( base != NULL );
  assert( base->name != NULL );

  if( sscanf( *line, "%s %d %d %x %u %u%n", name, &id, &sline, &(suppl.all), &pdim_num, &udim_num, &chars_read ) == 6 ) {

    *line = *line + chars_read;

    if( !scope_compare( base->name, name ) || (base->pdim_num != pdim_num) || (base->udim_num != udim_num) ) {

      print_output( "Attempting to merge two databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      /* Make sure that the exclude bit is merged */
      base->suppl.part.excluded |= suppl.part.excluded;

      i = 0;
      while( (i < (pdim_num + udim_num)) && (sscanf( *line, " %d %d%n", &msb, &lsb, &chars_read ) == 2) ) {
        *line = *line + chars_read;
        i++;
      }

      if( i == (pdim_num + udim_num) ) {

        /* Read in vector information */
        vector_db_merge( base->value, line, same );

      }

    }

  } else {

    print_output( "Unable to parse vsignal in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Merges two vsignals, placing the result into the base vsignal.  This function is used to calculate
 module coverage for the GUI.
*/
void vsignal_merge(
  vsignal* base,  /*!< Base vsignal that will contain the merged results */
  vsignal* other  /*!< Other vsignal that will be merged into the base vsignal */
) { PROFILE(VSIGNAL_MERGE);

  assert( base != NULL );
  assert( base->name != NULL );
  assert( scope_compare( base->name, other->name ) );
  //assert( base->id == other->id );
  assert( base->pdim_num == other->pdim_num );
  assert( base->udim_num == other->udim_num );

  /* Merge the exclusion information */
  base->suppl.part.excluded |= other->suppl.part.excluded;

  /* Read in vector information */
  vector_merge( base->value, other->value );

  PROFILE_END;

}

/*!
  When the specified signal in the parameter list has changed values, this function
  is called to propagate the value change to the simulator to cause any statements
  waiting on this value change to be resimulated.
*/
void vsignal_propagate(
  vsignal*        sig,  /*!< Pointer to signal to propagate change information from */
  const sim_time* time  /*!< Current simulation time when signal changed */
) { PROFILE(VSIGNAL_PROPAGATE);

  unsigned int i;

  /* Iterate through vsignal's expression list */
  for( i=0; i<sig->exp_size; i++ ) {

    expression* exp = sig->exps[i];

    /* Add to simulation queue if expression is a RHS, not a function call and not a port assignment */
    if( (exp->op != EXP_OP_FUNC_CALL) &&
        (exp->op != EXP_OP_PASSIGN) ) {
      sim_expr_changed( exp, time );
    }

  }

  PROFILE_END;

}

/*!
 \throws anonymous vector_vcd_assign vector_vcd_assign

 Assigns the associated value to the specified vsignal's vector.  After this, it
 iterates through its expression list, setting the TRUE and FALSE bits accordingly.
 Finally, calls the simulator expr_changed function for each expression.
*/
void vsignal_vcd_assign(
  vsignal*        sig,    /*!< Pointer to vsignal to assign VCD value to */
  const char*     value,  /*!< String version of VCD value */
  unsigned int    msb,    /*!< Most significant bit to assign to */
  unsigned int    lsb,    /*!< Least significant bit to assign to */
  const sim_time* time    /*!< Current simulation time signal is being assigned */
) { PROFILE(VSIGNAL_VCD_ASSIGN);

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
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Assigning vsignal %s[%d:%d] (lsb=%d) to value %s",
                                obf_sig( sig->name ), msb, lsb, sig->dim[0].lsb, value );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Set vsignal value to specified value */
  if( lsb > 0 ) {
    vec_changed = vector_vcd_assign( sig->value, value, (msb - sig->dim[0].lsb), (lsb - sig->dim[0].lsb) );
  } else {
    vec_changed = vector_vcd_assign( sig->value, value, msb, lsb );
  }

  /* Don't go through the hassle of updating expressions if value hasn't changed */
  if( vec_changed && !info_suppl.part.inlined ) {

    /* Propagate signal changes to rest of design */
    vsignal_propagate( sig, time );

  } 

  PROFILE_END;

}

/*!
 Adds the specified expression to the end of this vsignal's expression
 list.
*/
void vsignal_add_expression(
  vsignal*    sig,  /*!< Pointer to vsignal to add expression to */
  expression* expr  /*!< Expression to add to list */
) { PROFILE(VSIGNAL_ADD_EXPRESSION);

  exp_link_add( expr, &(sig->exps), &(sig->exp_size) );

  PROFILE_END;

}

/*!
 Displays vsignal's name, dimensional info, width and value vector to the standard output.
*/
void vsignal_display(
  vsignal* sig  /*!< Pointer to vsignal to display to standard output */
) {

  unsigned int i;  /* Loop iterator */

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

  switch( sig->value->suppl.part.data_type ) {
    case VDATA_UL  :  vector_display_value_ulong( sig->value->value.ul, sig->value->width );  break;
    case VDATA_R64 :  printf( "%.16lf", sig->value->value.r64->val );  break;
    case VDATA_R32 :  printf( "%.16f", sig->value->value.r32->val );  break;
    default        :  assert( 0 );  break;
  }

  printf( "\n" );

}

/*!
 \return Returns pointer to newly created vsignal structure, or returns
         NULL is specified string does not properly describe a vsignal.

 Converts the specified string describing a Verilog design vsignal.  The
 vsignal may be a standard vsignal name, a single bit select vsignal or a
 multi-bit select vsignal.
*/
vsignal* vsignal_from_string(
  /*@out@*/ char** str  /*!< String version of vsignal */
) { PROFILE(VSIGNAL_FROM_STRING);

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
    sig->dim        = (dim_range*)malloc_safe( sizeof( dim_range ) * 1 );
    sig->dim[0].msb = left;
    sig->dim[0].lsb = right;
    sig->suppl.part.big_endian = big_endian;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d+:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT_POS, right, 0, 0 );
    sig->pdim_num   = 1;
    sig->dim        = (dim_range*)malloc_safe( sizeof( dim_range ) * 1 );
    sig->dim[0].msb = left + right;
    sig->dim[0].lsb = left;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d-:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT_NEG, right, 0, 0 );
    sig->pdim_num   = 1;
    sig->dim        = (dim_range*)malloc_safe( sizeof( dim_range ) * 1 );
    sig->dim[0].msb = left - right;
    sig->dim[0].lsb = left;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d]%n", name, &right, &chars_read ) == 2 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, 0, 0 );
    sig->pdim_num   = 1;
    sig->dim        = (dim_range*)malloc_safe( sizeof( dim_range ) * 1 );
    sig->dim[0].msb = right;
    sig->dim[0].lsb = right;
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]%n", name, &chars_read ) == 1 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, 0, 0 );
    /* Specify that this width is unknown */
    vector_dealloc_value( sig->value );
    sig->value->width = 0;
    sig->value->value.ul = NULL;
    *str += chars_read;
  } else {
    sig = NULL;
  }

  PROFILE_END;

  return( sig );

}
#endif /* RUNLIB */

/*!
 \return Returns width of the given expression that is bound to the given signal.
*/
int vsignal_calc_width_for_expr(
  expression* expr,  /*!< Pointer to expression to get width for */
  vsignal*    sig    /*!< Pointer to signal to get width for */
) { PROFILE(VSIGNAL_CALC_WIDTH_FOR_EXPR);

  int          exp_dim;    /* Expression dimension number */
  int          width = 1;  /* Return value for this function */
  unsigned int i;          /* Loop iterator */

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

  PROFILE_END;

  return( width );

}

#ifndef RUNLIB
/*!
 \return Returns the LSB of the given signal for the given expression.
*/
int vsignal_calc_lsb_for_expr(
  expression* expr,    /*!< Pointer to expression to get LSB for */
  vsignal*    sig,     /*!< Pointer to signal to get LSB for */
  int         lsb_val  /*!< Calculated LSB value from this expression */
) { PROFILE(VSIGNAL_CALC_LSB_FOR_EXPR);

  int width = vsignal_calc_width_for_expr( expr, sig ) * lsb_val;

  PROFILE_END;

  return( width );

}
#endif /* RUNLIB */

/*!
 Deallocates all malloc'ed memory back to the heap for the specified
 vsignal.
*/
void vsignal_dealloc(
  /*@only@*/ vsignal* sig  /*!< Pointer to vsignal to deallocate */
) { PROFILE(VSIGNAL_DEALLOC);

  if( sig != NULL ) {

    unsigned int i;

    /* Free the signal name */
    free_safe( sig->name, (strlen( sig->name ) + 1) );
    sig->name = NULL;

    /* Free the dimension information */
    free_safe( sig->dim, (sizeof( dim_range ) * (sig->pdim_num + sig->udim_num)) );

    /* Free up memory for value */
    vector_dealloc( sig->value );
    sig->value = NULL;

    /* Free up memory for expression list */
    for( i=0; i<sig->exp_size; i++ ) {
      sig->exps[i]->sig = NULL;
    }

    exp_link_delete_list( sig->exps, sig->exp_size, FALSE );
    sig->exps     = NULL;
    sig->exp_size = 0;

    /* Finally free up the memory for this vsignal */
    free_safe( sig, sizeof( vsignal ) );

  }

  PROFILE_END;

}
