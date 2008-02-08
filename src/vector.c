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
 \file     vector.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/1/2001
 
 \par
 The vector is the data structure that stores all of the current value and coverage
 information for a particular signal or expression.  All simulated operations are
 performed on vector structures and as such are the most used and optimized data
 structures used by Covered.  To keep the memory image of a vector to as small as
 possible, a vector is comprised of three components:
 
 \par
   -# A least-significant bit (LSB) value which specifies the current vector's LSB.
   -# A width value which specifies the number of bits contained in the current vector.
      To get the MSB of the vector, simply add the width to the LSB and subtract one.
   -# A value array which contains the vector's current value and other coverage
      information gained during simulation.  This array is an array of 32-bit values
      (or nibbles) whose length is determined by the width of the vector divided by four.
      We divide the width by 4 because one nibble contains all of the information for
      up to 4 bits of four-state data.  For a break-down of the bits within a nibble,
      please consult the \ref nibble table.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>
#include <assert.h>

#ifdef MALLOC_DEBUG
#include <mpatrol.h>
#endif

#include "defines.h"
#include "vector.h"
#include "util.h"


/*@-initsize@*/
nibble xor_optab[OPTAB_SIZE]  = { XOR_OP_TABLE  };  /*!< XOR operation table */
nibble and_optab[OPTAB_SIZE]  = { AND_OP_TABLE  };  /*!< AND operation table */
nibble or_optab[OPTAB_SIZE]   = { OR_OP_TABLE   };  /*!< OR operation table */
nibble nand_optab[OPTAB_SIZE] = { NAND_OP_TABLE };  /*!< NAND operation table */
nibble nor_optab[OPTAB_SIZE]  = { NOR_OP_TABLE  };  /*!< NOR operation table */
nibble nxor_optab[OPTAB_SIZE] = { NXOR_OP_TABLE };  /*!< NXOR operation table */
/*@=initsize@*/

extern char user_msg[USER_MSG_LENGTH];


/*!
 \param vec         Pointer to vector to initialize.
 \param value       Pointer to vec_data array for vector.
 \param owns_value  Set to TRUE if this vector is responsible for deallocating the given value array
 \param width       Bit width of specified vector.
 \param type        Type of vector to initialize this to.
 
 Initializes the specified vector with the contents of width
 and value (if value != NULL).  If value != NULL, initializes all contents 
 of value array to 0x2 (X-value).
*/
void vector_init( vector* vec, vec_data* value, bool owns_value, int width, int type ) { PROFILE(VECTOR_INIT);

  vec->width                = width;
  vec->suppl.all            = 0;
  vec->suppl.part.type      = type;
  vec->suppl.part.owns_data = owns_value;
  vec->value                = value;

  if( value != NULL ) {

    int i;  /* Loop iterator */

    assert( width > 0 );

    for( i=0; i<width; i++ ) {
      vec->value[i].all = 0x0;
    }

  } else {

    assert( !owns_value );

  }

  PROFILE_END;

}

/*!
 \param width  Bit width of this vector.
 \param type   Type of vector to create (see \ref vector_types for valid values).
 \param data   If FALSE only initializes width but does not allocate a nibble array.

 \return Pointer to newly created vector.

 Creates new vector from heap memory and initializes all vector contents.
*/
vector* vector_create( int width, int type, bool data ) { PROFILE(VECTOR_CREATE);

  vector*   new_vec;       /* Pointer to newly created vector */
  vec_data* value = NULL;  /* Temporarily stores newly created vector value */

  assert( width > 0 );

  new_vec = (vector*)malloc_safe( sizeof( vector ) );

  if( data == TRUE ) {
#ifdef SKIP
    if( width > MAX_BIT_WIDTH ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Found a vector width (%d) that exceeds the maximum currently allowed by Covered (%d)",
                                  width, MAX_BIT_WIDTH );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( EXIT_FAILURE );
    }
#endif
    value = (vec_data*)malloc_safe( sizeof( vec_data ) * width );
  }

  vector_init( new_vec, value, (value != NULL), width, type );

  PROFILE_END;

  return( new_vec );

}

/*!
 \param from_vec  Vector to copy.
 \param to_vec    Newly created vector copy.
 
 Copies the contents of the from_vec to the to_vec, allocating new memory.
*/
void vector_copy( vector* from_vec, vector** to_vec ) { PROFILE(VECTOR_COPY);

  int i;  /* Loop iterator */

  if( from_vec == NULL ) {

    /* If from_vec is NULL, just assign to_vec to NULL */
    *to_vec = NULL;

  } else {

    /* Create vector */
    *to_vec = vector_create( from_vec->width, from_vec->suppl.part.type, TRUE );

    /* Copy contents of value array */
    for( i=0; i<from_vec->width; i++ ) {
      (*to_vec)->value[i] = from_vec->value[i];
    }

  }

  PROFILE_END;

}

/*!
 \param dat0  Data nibble 0
 \param dat1  Data nibble 1
 \param dat2  Data nibble 2
 \param dat3  Data nibble 3

 \return Returns an unsigned integer containing the values of dat0, dat1, dat2 and dat3
         in an encoded, packed manner.
*/
static unsigned int vector_nibbles_to_uint( nibble dat0, nibble dat1, nibble dat2, nibble dat3 ) { PROFILE(VECTOR_NIBBLES_TO_UINT);

  unsigned int d[4];  /* Array of unsigned int format of dat0,1,2,3 */
  unsigned int i;     /* Loop iterator */

  d[0] = ((unsigned int)dat0) & 0xff;
  d[1] = ((unsigned int)dat1) & 0xff;
  d[2] = ((unsigned int)dat2) & 0xff;
  d[3] = ((unsigned int)dat3) & 0xff;

  for( i=0; i<4; i++ ) {
    d[i] = ( ((d[i] & 0x03) << (i *  2)) |
             ((d[i] & 0x04) << (i +  6)) |
             ((d[i] & 0x08) << (i +  9)) |
             ((d[i] & 0x10) << (i + 12)) |
             ((d[i] & 0x20) << (i + 15)) |
             ((d[i] & 0x40) << (i + 18)) |
             ((d[i] & 0x80) << (i + 21)) );
  }

  PROFILE_END;

  return( d[0] | d[1] | d[2] | d[3] );

}

/*!
 \param data  Unsigned integer value containing the data to be nibble-ized
 \param dat   Array of four nibbles to populate

 Decodes and unpacks the given unsigned integer value into the specified nibble array.
*/
static void vector_uint_to_nibbles( unsigned int data, nibble* dat ) { PROFILE(VECTOR_UINT_TO_NIBBLES);

  unsigned int i;  /* Loop iterator */

  for( i=0; i<4; i++ ) {
    dat[i] = (nibble)( (((data & (0x00000003 << (i * 2))) >> ((i * 2) + 0)) |
                        ((data & (0x00000100 << i)      ) >> (i +  6)) |
                        ((data & (0x00001000 << i)      ) >> (i +  9)) |
                        ((data & (0x00010000 << i)      ) >> (i + 12)) |
                        ((data & (0x00100000 << i)      ) >> (i + 15)) |
                        ((data & (0x01000000 << i)      ) >> (i + 18)) |
                        ((data & (0x10000000 << i)      ) >> (i + 21))) & 0xff );
  }

  PROFILE_END;

}

/*!
 \param vec         Pointer to vector to display to database file.
 \param file        Pointer to coverage database file to display to.
 \param write_data  If set to TRUE, causes 4-state data bytes to be included.

 Writes the specified vector to the specified coverage database file.
*/
void vector_db_write( vector* vec, FILE* file, bool write_data ) { PROFILE(VECTOR_DB_WRITE);

  int    i;      /* Loop iterator */
  nibble mask;   /* Mask value for vector value nibbles */
  nibble dflt;   /* Default value (based on 2 or 4 state) */

  assert( vec != NULL );
  assert( vec->width > 0 );

  /* Calculate vector data mask */
  mask = write_data ? 0xff : 0xfc;
  switch( vec->suppl.part.type ) {
    case VTYPE_VAL :  mask = mask & 0x03;  break;
    case VTYPE_SIG :  mask = mask & 0x0f;  break;
    case VTYPE_EXP :  mask = mask & 0x3f;  break;
    case VTYPE_MEM :  mask = mask & 0x3f;  break;
    default        :  break;
  }

  /* Calculate default value of bit */
  dflt = (vec->suppl.part.is_2state == 1) ? 0x0 : 0x2;

  /* Set owns_data supplemental bit in all cases */
  vec->suppl.part.owns_data = 1;

  /* Output vector information to specified file */
  /*@-formatcode@*/
  fprintf( file, "%d %hhu",
    vec->width,
    vec->suppl.all
  );
  /*@=formatcode@*/

  if( vec->value == NULL ) {

    /* If the vector value was NULL, output default value */
    for( i=0; i<vec->width; i+=4 ) {
      switch( vec->width - i ) {
        case 0 :  break;
        case 1 :  fprintf( file, " %x", vector_nibbles_to_uint( dflt, 0x0,  0x0,  0x0 ) );   break;
        case 2 :  fprintf( file, " %x", vector_nibbles_to_uint( dflt, dflt, 0x0,  0x0 ) );   break;
        case 3 :  fprintf( file, " %x", vector_nibbles_to_uint( dflt, dflt, dflt, 0x0 ) );   break;
        default:  fprintf( file, " %x", vector_nibbles_to_uint( dflt, dflt, dflt, dflt ) );  break;
      }
    }

  } else {

    for( i=0; i<vec->width; i+=4 ) {
      switch( vec->width - i ) {
        case 0 :  break;
        case 1 :
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0].all & mask) | (write_data ? 0 : dflt)), 0, 0, 0 ) );
          break;
        case 2 :
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0].all & mask) | (write_data ? 0 : dflt)),
                                                        ((vec->value[i+1].all & mask) | (write_data ? 0 : dflt)), 0, 0 ) );
          break;
        case 3 :
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0].all & mask) | (write_data ? 0 : dflt)), 
                                                        ((vec->value[i+1].all & mask) | (write_data ? 0 : dflt)),
                                                        ((vec->value[i+2].all & mask) | (write_data ? 0 : dflt)), 0 ) );
          break;
        default:
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0].all & mask) | (write_data ? 0 : dflt)), 
                                                        ((vec->value[i+1].all & mask) | (write_data ? 0 : dflt)),
                                                        ((vec->value[i+2].all & mask) | (write_data ? 0 : dflt)),
                                                        ((vec->value[i+3].all & mask) | (write_data ? 0 : dflt)) ) );
          break;
      }
    }

  }

  PROFILE_END;

}

/*!
 \param vec    Pointer to vector to create.
 \param line   Pointer to line to parse for vector information.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Creates a new vector structure, parses current file line for vector information
 and returns new vector structure to calling function.
*/
bool vector_db_read( vector** vec, char** line ) { PROFILE(VECTOR_DB_READ);

  bool         retval = TRUE;  /* Return value for this function */
  int          width;          /* Vector bit width */
  int          suppl;          /* Temporary supplemental value */
  int          i;              /* Loop iterator */
  int          chars_read;     /* Number of characters read */
  unsigned int value;          /* Temporary value */
  nibble       nibs[4];        /* Temporary nibble value containers */

  /* Read in vector information */
  if( sscanf( *line, "%d %d%n", &width, &suppl, &chars_read ) == 2 ) {

    *line = *line + chars_read;

    /* Create new vector */
    *vec              = vector_create( width, VTYPE_VAL, TRUE );
    (*vec)->suppl.all = (char)suppl & 0xff;

    i = 0;
    while( (i < width) && retval ) {
      if( sscanf( *line, "%x%n", &value, &chars_read ) == 1 ) {
        *line += chars_read;
        vector_uint_to_nibbles( value, nibs );
        switch( width - i ) {
          case 0 :  break;
          case 1 :
            (*vec)->value[i+0].all = nibs[0];
            break;
          case 2 :
            (*vec)->value[i+0].all = nibs[0];
            (*vec)->value[i+1].all = nibs[1];
            break;
          case 3 :
            (*vec)->value[i+0].all = nibs[0];
            (*vec)->value[i+1].all = nibs[1];
            (*vec)->value[i+2].all = nibs[2];
            break;
          default:
            (*vec)->value[i+0].all = nibs[0];
            (*vec)->value[i+1].all = nibs[1];
            (*vec)->value[i+2].all = nibs[2];
            (*vec)->value[i+3].all = nibs[3];
            break;
        }
      } else {
        retval = FALSE;
      }
      i += 4;
    }

  } else {

    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param base  Base vector to merge data into.
 \param line  Pointer to line to parse for vector information.
 \param same  Specifies if vector to merge needs to be exactly the same as the existing vector.

 Parses current file line for vector information and performs vector merge of 
 base vector and read vector information.  If the vectors are found to be different
 (width is not equal), an error message is sent to the user and the
 program is halted.  If the vectors are found to be equivalents, the merge is
 performed on the vector nibbles.
*/
void vector_db_merge( vector* base, char** line, bool same ) { PROFILE(VECTOR_DB_MERGE);

  int          width;           /* Width of read vector */
  int          suppl;           /* Supplemental value of vector */
  int          chars_read;      /* Number of characters read */
  int          i;               /* Loop iterator */
  unsigned int value;           /* Integer form of value */
  nibble       nibs[4];         /* Temporary nibble containers */

  assert( base != NULL );

  if( sscanf( *line, "%d %d%n", &width, &suppl, &chars_read ) == 2 ) {

    *line = *line + chars_read;

    if( base->width != width ) {

      if( same ) {
        print_output( "Attempting to merge databases derived from different designs.  Unable to merge",
                      FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

    } else {

      i = 0;
      while( i < width ) {
        if( sscanf( *line, "%x%n", &value, &chars_read ) == 1 ) {
          *line += chars_read;
          vector_uint_to_nibbles( value, nibs );
          switch( width - i ) {
            case 0 :  break;
            case 1 :  
              base->value[i+0].all = (base->value[i+0].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              break;
            case 2 :
              base->value[i+0].all = (base->value[i+0].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              base->value[i+1].all = (base->value[i+1].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[1] & VECTOR_MERGE_MASK);
              break;
            case 3 :
              base->value[i+0].all = (base->value[i+0].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              base->value[i+1].all = (base->value[i+1].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[1] & VECTOR_MERGE_MASK);
              base->value[i+2].all = (base->value[i+2].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[2] & VECTOR_MERGE_MASK);
              break;
            default:
              base->value[i+0].all = (base->value[i+0].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              base->value[i+1].all = (base->value[i+1].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[1] & VECTOR_MERGE_MASK);
              base->value[i+2].all = (base->value[i+2].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[2] & VECTOR_MERGE_MASK);
              base->value[i+3].all = (base->value[i+3].all & (VECTOR_MERGE_MASK | 0x3)) | (nibs[3] & VECTOR_MERGE_MASK);
              break;
          }
        } else {
          print_output( "Unable to parse vector line from database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
        i += 4;
      }
                       
    }

  } else {

    print_output( "Unable to parse vector line from database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param nib    Pointer to vector data array to get string from
 \param width  Width of given vector data array

 \return Returns a string showing the toggle 0 -> 1 information.
*/
char* vector_get_toggle01( vec_data* nib, int width ) { PROFILE(VECTOR_GET_TOGGLE01);

  char* bits = (char*)malloc_safe( width + 1 );
  int   i;
  char  tmp[2];

  for( i=(width - 1); i>=0; i-- ) {
    /*@-formatcode@*/
    unsigned int rv = snprintf( tmp, 2, "%hhx", nib[i].part.sig.tog01 );
    /*@=formatcode@*/
    assert( rv < 2 );
    bits[((width - 1) - i)] = tmp[0];
  }

  bits[width] = '\0';

  PROFILE_END;

  return( bits );

}

/*!
 \param nib    Pointer to vector data array to get string from
 \param width  Width of given vector data array

 \return Returns a string showing the toggle 1 -> 0 information.
*/
char* vector_get_toggle10( vec_data* nib, int width ) { PROFILE(VECTOR_GET_TOGGLE10);

  char* bits = (char*)malloc_safe( width + 1 );
  int   i; 
  char  tmp[2];

  for( i=(width - 1); i>=0; i-- ) {
    /*@-formatcode@*/
    unsigned int rv = snprintf( tmp, 2, "%hhx", nib[i].part.sig.tog10 );
    /*@=formatcode@*/
    assert( rv < 2 );
    bits[((width - 1) - i)] = tmp[0];
  }

  bits[width] = '\0';

  PROFILE_END;

  return( bits );

}

/*!
 \param nib    Nibble to display toggle information
 \param width  Number of bits of nibble to display
 \param ofile  Stream to output information to.
 
 Displays the toggle01 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle01( vec_data* nib, int width, FILE* ofile ) { PROFILE(VECTOR_DISPLAY_TOGGLE01);

  unsigned int value = 0;  /* Current 4-bit hexidecimal value of toggle */
  int          i;          /* Loop iterator */

  fprintf( ofile, "%d'h", width );

  for( i=(width - 1); i>=0; i-- ) {
    value = value | (nib[i].part.sig.tog01 << ((unsigned)i % 4));
    if( (i % 4) == 0 ) {
      fprintf( ofile, "%1x", value );
      value = 0;
    }
    if( ((i % 16) == 0) && (i != 0) ) {
      fprintf( ofile, "_" );
    }
  }

  PROFILE_END;

}

/*!
 \param nib    Nibble to display toggle information
 \param width  Number of bits of nibble to display
 \param ofile  Stream to output information to.
 
 Displays the toggle10 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle10( vec_data* nib, int width, FILE* ofile ) { PROFILE(VECTOR_DISPLAY_TOGGLE10);

  unsigned int value = 0;  /* Current 4-bit hexidecimal value of toggle */
  int          i;          /* Loop iterator */

  fprintf( ofile, "%d'h", width );

  for( i=(width - 1); i>=0; i-- ) {
    value = value | (nib[i].part.sig.tog10 << ((unsigned)i % 4));
    if( (i % 4) == 0 ) {
      fprintf( ofile, "%1x", value );
      value = 0;
    }
    if( ((i % 16) == 0) && (i != 0) ) {
      fprintf( ofile, "_" );
    }
  }

  PROFILE_END;

}

/*!
 \param nib    Pointer to vector nibble array
 \param width  Number of elements in nib array

 Displays the binary value of the specified vector data array to standard output.
*/
void vector_display_value( vec_data* nib, int width ) {

  int i;  /* Loop iterator */

  printf( "value: %d'b", width );

  for( i=(width - 1); i>=0; i-- ) {
    switch( nib[i].part.val.value ) {
      case 0 :  printf( "0" );  break;
      case 1 :  printf( "1" );  break;
      case 2 :  printf( "x" );  break;
      case 3 :  printf( "z" );  break;
      default:  break;
    }
  }

}

/*!
 \param nib    Nibble to display
 \param width  Number of bits in nibble to display
 \param type   Type of vector to display

 Outputs the specified nibble array to standard output as described by the
 width parameter.
*/
void vector_display_nibble( vec_data* nib, int width, int type ) {

  int i;  /* Loop iterator */

  printf( "\n" );
  printf( "      raw value:" );
  
  for( i=(width - 1); i>=0; i-- ) {
    /*@-formatcode@*/
    printf( " %hhx", nib[i].all );
    /*@=formatcode@*/
  }

  /* Display nibble value */
  printf( ", " );
  vector_display_value( nib, width );

  switch( type ) {

    case VTYPE_SIG :

      /* Display nibble toggle01 history */
      printf( ", 0->1: " );
      vector_display_toggle01( nib, width, stdout );

      /* Display nibble toggle10 history */
      printf( ", 1->0: " );
      vector_display_toggle10( nib, width, stdout );

      /* Display bit set information */
      printf( ", set: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.sig.set );
        /*@=formatcode@*/
      }

      break;

    case VTYPE_EXP :

      /* Display eval_a information */
      printf( ", a: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.exp.eval_a );
        /*@=formatcode@*/
      }

      /* Display eval_b information */
      printf( ", b: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.exp.eval_b );
        /*@=formatcode@*/
      }

      /* Display eval_c information */
      printf( ", c: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.exp.eval_c );
        /*@=formatcode@*/
      }

      /* Display eval_d information */
      printf( ", d: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.exp.eval_d );
        /*@=formatcode@*/
      }

      /* Display set information */
      printf( ", set: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.exp.set );
        /*@=formatcode@*/
      }

      break;

    case VTYPE_MEM :
  
      /* Display nibble toggle01 history */
      printf( ", 0->1: " );
      vector_display_toggle01( nib, width, stdout );

      /* Display nibble toggle10 history */
      printf( ", 1->0: " );
      vector_display_toggle10( nib, width, stdout );

      /* Write history */
      printf( ", wr: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.mem.wr );
        /*@=formatcode@*/
      }

      /* Read history */
      printf( ", rd: %d'b", width );
      for( i=(width - 1); i>=0; i-- ) {
        /*@-formatcode@*/
        printf( "%hhu", nib[i].part.mem.rd );
        /*@=formatcode@*/
      }

      break;

    default : break;

  }

}

/*!
 \param vec  Pointer to vector to output to standard output.

 Outputs contents of vector to standard output (for debugging purposes only).
*/
void vector_display( vector* vec ) {

  assert( vec != NULL );

  /*@-formatcode@*/
  printf( "Vector => width: %d, suppl: %hhx\n", vec->width, vec->suppl.all );
  /*@=formatcode@*/

  if( (vec->width > 0) && (vec->value != NULL) ) {
    vector_display_nibble( vec->value, vec->width, vec->suppl.part.type );
  } else {
    printf( "NO DATA" );
  }

  printf( "\n" );

}

/*!
 \param vec        Pointer to vector to parse.
 \param tog01_cnt  Number of bits in vector that toggled from 0 to 1.
 \param tog10_cnt  Number of bits in vector that toggled from 1 to 0.

 Walks through specified vector counting the number of toggle01 bits that
 are set and the number of toggle10 bits that are set.  Adds these values
 to the contents of tog01_cnt and tog10_cnt.
*/
void vector_toggle_count( vector* vec, int* tog01_cnt, int* tog10_cnt ) { PROFILE(VECTOR_TOGGLE_COUNT);

  int i;  /* Loop iterator */

  if( (vec->suppl.part.type == VTYPE_SIG) || (vec->suppl.part.type == VTYPE_MEM) ) {

    for( i=0; i<vec->width; i++ ) {
      *tog01_cnt = *tog01_cnt + vec->value[i].part.sig.tog01;
      *tog10_cnt = *tog10_cnt + vec->value[i].part.sig.tog10;
    }

  }

  PROFILE_END;

}

/*!
 \param vec     Pointer to vector to parse
 \param wr_cnt  Pointer to number of bits in vector that were written
 \param rd_cnt  Pointer to number of bits in vector that were read

 Counts the number of bits that were written and read for the given memory
 vector.
*/
void vector_mem_rw_count( vector* vec, int* wr_cnt, int* rd_cnt ) { PROFILE(VECTOR_MEM_RW_COUNT);

  int i;  /* Loop iterator */

  for( i=0; i<vec->width; i++ ) {
    *wr_cnt += vec->value[i].part.mem.wr;
    *rd_cnt += vec->value[i].part.mem.rd;
  }

  PROFILE_END;

}

/*!
 \param vec  Pointer to vector value to set
 \param msb  Most-significant bit to set in vector value
 \param lsb  Least-significant bit to set in vector value

 \return Returns TRUE if assigned bit that is being set to 1 in this function was
         found to be previously set; otherwise, returns FALSE.

 Sets the assigned supplemental bit for the given bit range in the given vector.  Called by
 race condition checker code.
*/
bool vector_set_assigned(
  vector* vec,
  int msb,
  int lsb
) { PROFILE(VECTOR_SET_ASSIGNED);

  bool prev_assigned = FALSE;  /* Specifies if any set bit was previously set */
  int  i;                      /* Loop iterator */

  assert( vec != NULL );
  assert( (msb - lsb) < vec->width );
  assert( vec->suppl.part.type == VTYPE_SIG );

  for( i=lsb; i<=msb; i++ ) {
    if( vec->value[i].part.sig.misc == 1 ) {
      prev_assigned = TRUE;
    }
    vec->value[i].part.sig.misc = 1;
  }

  PROFILE_END;

  return( prev_assigned );

}

/*!
 \param vec       Pointer to vector to set value to.
 \param value     New value to set vector value to.
 \param width     Width of new value.
 \param from_idx  Starting bit index of value to start copying.
 \param to_idx    Starting bit index of value to copy to.

 \return Returns TRUE if assignment was performed; otherwise, returns FALSE.

 Allows the calling function to set any bit vector within the vector
 range.  If the vector value has never been set, sets
 the value to the new value and returns.  If the vector value has previously
 been set, checks to see if new vector bits have toggled, sets appropriate
 toggle values, sets the new value to this value and returns.
*/
bool vector_set_value(
  vector*   vec,
  vec_data* value,
  int       width,
  int       from_idx,
  int       to_idx
) { PROFILE(VECTOR_SET_VALUE);

  bool      retval = FALSE;  /* Return value for this function */
  nibble    from_val;        /* Current bit value of value being assigned */
  nibble    to_val;          /* Current bit value of previous value */
  int       i;               /* Loop iterator */
  vec_data  set_val;         /* Value to set current vec value to */
  vec_data* vval;            /* Pointer to vector value array */
  nibble    v2st;            /* Value to AND with from value bit if the target is a 2-state value */

  assert( vec != NULL );

  vval = vec->value;
  v2st = vec->suppl.part.is_2state << 1;

  /* Verify that index is within range */
  assert( to_idx < vec->width );
  assert( to_idx >= 0 );

  /* Adjust width to smaller of two values */
  width = (width > (vec->width - to_idx)) ? (vec->width - to_idx) : width;

  switch( vec->suppl.part.type ) {
    case VTYPE_VAL :
      for( i=0; i<width; i++ ) {
        vval[i + to_idx].part.val.value = ((v2st & value[i + from_idx].part.val.value) > 1) ? 0 : value[i + from_idx].part.val.value;
      }
      retval = TRUE;
      break;
    case VTYPE_SIG :
      for( i=width; i--; ) {
        set_val  = vval[i + to_idx];
        from_val = ((v2st & value[i + from_idx].part.val.value) > 1) ? 0 : value[i + from_idx].part.val.value;
        to_val   = set_val.part.sig.value;
        if( (from_val != to_val) || (set_val.part.sig.set == 0x0) ) {
          if( set_val.part.sig.set == 0x1 ) {
            /* Assign toggle values if necessary */
            if( (to_val == 0) && (from_val == 1) ) {
              set_val.part.sig.tog01 = 1;
            } else if( (to_val == 1) && (from_val == 0) ) {
              set_val.part.sig.tog10 = 1;
            }
          }
          /* Perform value assignment */
          set_val.part.sig.set   = 1;
          set_val.part.sig.value = from_val;
          vval[i + to_idx]       = set_val;
          retval = TRUE;
        }
      }
      break;
    case VTYPE_MEM :
      for( i=width; i--; ) {
        set_val  = vval[i + to_idx];
        from_val = ((v2st & value[i + from_idx].part.val.value) > 1) ? 0 : value[i + from_idx].part.val.value;
        to_val   = set_val.part.mem.value;
        if( from_val != to_val ) {
          /* Assign toggle values if necessary */
          if( (to_val == 0) && (from_val == 1) ) {
            set_val.part.mem.tog01 = 1;
          } else if( (to_val == 1) && (from_val == 0) ) {
            set_val.part.mem.tog10 = 1;
          }
          /* Assign write information */
          set_val.part.mem.wr = 1;
          /* Perform value assignment */
          set_val.part.mem.value = from_val;
          vval[i + to_idx]       = set_val;
          retval = TRUE;
        }
      }
      break;
    case VTYPE_EXP :
      for( i=width; i--; ) {
        set_val  = vval[i + to_idx];
        from_val = ((v2st & value[i + from_idx].part.val.value) > 1) ? 0 : value[i + from_idx].part.val.value;
        to_val   = set_val.part.exp.value;
        if( (from_val != to_val) || (set_val.part.exp.set == 0x0) ) {
          /* Perform value assignment */
          set_val.part.exp.set   = 1;
          set_val.part.exp.value = from_val;
          vval[i + to_idx]       = set_val;
          retval = TRUE;
        }
      }
      break;
    default : break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param vec  Pointer to vector to bit-fill
 \param msb  Most-significant bit to end bit-filling on
 \param lsb  Least-significant bit to start bit-filling

 \return Returns TRUE if any of the bits in the bit-fill range have changed
         value.

 Performs a bit-fill of the specified vector starting at the specified LSB
 and bit-filling all bits to the MSB.
*/
bool vector_bit_fill( vector* vec, int msb, int lsb ) { PROFILE(VECTOR_BIT_FILL);

  vec_data value;            /* Temporary vector data value */
  bool     changed = FALSE;  /* Return value for this function */
  int      i;                /* Loop iterator */

  assert( vec != NULL );
  assert( lsb > 0 ); 
  msb = (msb > vec->width) ? vec->width : msb;

  switch( vec->value[lsb-1].part.val.value ) {
    case 0 :  value.all = 0;  break;
    case 1 :  value.all = 0;  break;
    case 2 :  value.all = 2;  break;
    case 3 :  value.all = 3;  break;
    default:  break;
  }

  for( i=lsb; i<msb; i++ ) {
    changed |= vector_set_value( vec, &value, 1, 0, i );
  }

  PROFILE_END;

  return( changed );

}

/*!
 \param vec  Pointer to vector to check for unknowns.

 \return Returns TRUE if the specified vector contains unknown values; otherwise, returns FALSE.

 Checks specified vector for any X or Z values and returns TRUE if any are found; otherwise,
 returns a value of false.  This function is useful to be called before vector_to_int is called.
*/
bool vector_is_unknown( const vector* vec ) { PROFILE(VECTOR_IS_UNKNOWN);

  bool unknown = FALSE;  /* Specifies if vector contains unknown values */
  int  i;                /* Loop iterator */
  int  val;              /* Bit value of current bit */

  assert( vec != NULL );
  assert( vec->width > 0 );
  assert( vec->value != NULL );

  for( i=0; i<vec->width; i++ ) {
    val = vec->value[i].part.val.value;
    if( (val == 0x2) || (val == 0x3) ) {
      unknown = TRUE;
    }
  }

  PROFILE_END;

  return( unknown );

}

/*!
 \param vec  Pointer to vector to check for set bits

 \return Returns TRUE if the specified vector has been previously set (simulated); otherwise,
         returns FALSE.
*/
bool vector_is_set( vector* vec ) { PROFILE(VECTOR_IS_SET);

  int i = 0;  /* Loop iterator */

  assert( vec != NULL );
  assert( vec->value != NULL );

  while( (i < vec->width) && (vec->value[i].part.sig.set == 0) ) i++;

  PROFILE_END;

  return( i < vec->width );

}

/*!
 \param vec  Pointer to vector to convert into integer.

 \return Returns integer value of specified vector.

 Converts a vector structure into an integer value.  If the number of bits for the
 vector exceeds the number of bits in an integer, the upper bits of the vector are
 unused.
*/
int vector_to_int( vector* vec ) { PROFILE(VECTOR_TO_INT);

  int retval = 0;   /* Integer value returned to calling function */
  int i;            /* Loop iterator */
  int width;        /* Number of bits to use in creating integer */

  width = (vec->width > (SIZEOF_INT * 8)) ? 32 : vec->width;

  for( i=(width - 1); i>=0; i-- ) {
    switch( vec->value[i].part.val.value ) {
      /*@-shiftimplementation@*/
      case 0 :  retval = (retval << 1) | 0;  break;
      case 1 :  retval = (retval << 1) | 1;  break;
      /*@=shiftimplementation@*/
      default:
        print_output( "Vector converting to integer contains X or Z values", FATAL, __FILE__, __LINE__ );
        assert( vec->value[i].part.val.value < 2 );
        break;
    }
  }

  /* If the vector is signed, sign-extend the integer */
  if( vec->suppl.part.is_signed == 1 ) {
    for( i=width; i<(SIZEOF_INT * 8); i++ ) {
      /*@-shiftnegative@*/
      retval |= (vec->value[width-1].part.val.value << i);
      /*@=shiftnegative@*/
    }
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param vec  Pointer to vector to convert into integer.

 \return Returns integer value of specified vector.

 Converts a vector structure into an integer value.  If the number of bits for the
 vector exceeds the number of bits in an integer, the upper bits of the vector are
 unused.
*/
uint64 vector_to_uint64( vector* vec ) { PROFILE(VECTOR_TO_UINT64);

  uint64 retval = 0;   /* 64-bit integer value returned to calling function */
  int    i;            /* Loop iterator */
  int    width;        /* Number of bits to use in creating integer */

  width = (vec->width > 64) ? 64 : vec->width;

  for( i=(width - 1); i>=0; i-- ) {
    switch( vec->value[i].part.val.value ) {
      case 0 :  retval = (retval << 1);  break;
      case 1 :  retval = (retval << 1) | 1;  break;
      default:
        print_output( "Vector converting to 64-bit integer contains X or Z values", FATAL, __FILE__, __LINE__ );
        assert( vec->value[i].part.val.value < 2 );
        break;
    }
  }

  /* If the vector is signed, sign-extend the integer */
  if( vec->suppl.part.is_signed == 1 ) {
    for( i=width; i<64; i++ ) {
      /*@-shiftnegative@*/
      retval |= (vec->value[width-1].part.val.value << i);
      /*@=shiftnegative@*/
    }
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param vec   Pointer to vector to convert into integer.
 \param time  Pointer to sim_time structure to populate.

 Converts a vector structure into a sim_time structure.  If the number of bits for the
 vector exceeds the number of bits in an 64-bit integer, the upper bits of the vector are
 unused.
*/
void vector_to_sim_time( vector* vec, sim_time* time ) { PROFILE(VECTOR_TO_SIM_TIME);

  int width;  /* Number of bits to use in creating sim_time */
  int i;      /* Loop iterator */

  width = (vec->width > 64) ? 64 : vec->width;

  for( i=(width - 1); i--; ) {
    switch( vec->value[i].part.val.value ) {
      case 0 :
        time->lo   = (i <  32) ? (time->lo << 1) : time->lo;
        time->hi   = (i >= 32) ? (time->hi << 1) : time->hi;
        time->full = (time->full << 1);
        break;
      case 1 :
        time->lo   = (i <  32) ? (time->lo << 1) | 1 : time->lo;
        time->hi   = (i >= 32) ? (time->hi << 1) | 1 : time->hi;
        time->full = (time->full << 1) | UINT64(1);
        break;
      default :
        print_output( "Vector converting to sim_time contains X or Z values", FATAL, __FILE__, __LINE__ );
        assert( vec->value[i].part.val.value < 2 );
        break;
    }
  }

  PROFILE_END;

}

/*!
 \param vec    Pointer to vector store value into.
 \param value  Integer value to convert into vector.

 Converts an integer value into a vector, creating a vector value to store the
 new vector into.  This function is used along with the vector_to_int for
 mathematical vector operations.  We will first convert vectors into integers,
 perform the mathematical operation, and then revert the integers back into
 the vectors.
*/
void vector_from_int( vector* vec, int value ) { PROFILE(VECTOR_FROM_INT);

  int width;  /* Number of bits to convert */
  int i;      /* Loop iterator */

  width = (vec->width < (SIZEOF_INT * 8)) ? vec->width : (SIZEOF_INT * 8);

  for( i=0; i<width; i++ ) {
    vec->value[i].part.val.value = (value & 0x1);
    /*@-shiftimplementation@*/
    value >>= 1;
    /*@=shiftimplementation@*/
  }

  /* Because this value came from an integer, specify that the vector is signed */
  vec->suppl.part.is_signed = 1;

  PROFILE_END;

}

/*!
 \param vec    Pointer to vector store value into.
 \param value  64-bit integer value to convert into vector.

 Converts a 64-bit integer value into a vector.  This function is used along with
 the vector_to_uint64 for mathematical vector operations.  We will first convert
 vectors into 64-bit integers, perform the mathematical operation, and then revert
 the 64-bit integers back into the vectors.
*/
void vector_from_uint64( vector* vec, uint64 value ) { PROFILE(VECTOR_FROM_UINT64);

  int width;  /* Number of bits to convert */
  int i;      /* Loop iterator */

  width = (vec->width < 64) ? vec->width : 64;

  for( i=0; i<width; i++ ) {
    vec->value[i].part.val.value = (value & 0x1);
    /*@-shiftimplementation@*/
    value >>= 1;
    /*@=shiftimplementation@*/
  }

  /* Because this value came from an unsigned integer, specify that the vector is unsigned */
  vec->suppl.part.is_signed = 0;

  PROFILE_END;

}

/*!
 \param vec            Pointer to vector to add static value to.
 \param str            Value string to add.
 \param bits_per_char  Number of bits necessary to store a value character (1, 3, or 4).

 Iterates through string str starting at the left-most character, calculates the int value
 of the character and sets the appropriate number of bits in the specified vector locations.
*/
static void vector_set_static( vector* vec, char* str, int bits_per_char ) { PROFILE(VECTOR_SET_STATIC);

  char*        ptr;  /* Pointer to current character evaluating */
  unsigned int pos;  /* Current bit position in vector */
  unsigned int val;  /* Temporary holder for value of current character */
  unsigned int i;    /* Loop iterator */

  pos = 0;

  ptr = str + (strlen( str ) - 1);

  while( ptr >= str ) {
    if( *ptr != '_' ) {
      if( (*ptr == 'x') || (*ptr == 'X') ) {
        for( i=0; i<bits_per_char; i++ ) {
          if( (i + pos) < vec->width ) { 
            vec->value[i + pos].part.val.value = 0x2;
          }
        }
      } else if( (*ptr == 'z') || (*ptr == 'Z') || (*ptr == '?') ) {
        for( i=0; i<bits_per_char; i++ ) {
          if( (i + pos) < vec->width ) { 
            vec->value[i + pos].part.val.value = 0x3;
          }
        }
      } else {
        if( (*ptr >= 'a') && (*ptr <= 'f') ) {
          val = (*ptr - 'a') + 10;
        } else if( (*ptr >= 'A') && (*ptr <= 'F') ) {
          val = (*ptr - 'A') + 10;
        } else {
          assert( (*ptr >= '0') && (*ptr <= '9') );
	  val = *ptr - '0';
        }
        for( i=0; i<bits_per_char; i++ ) {
          if( (i + pos) < vec->width ) {
            vec->value[i + pos].part.val.value = ((val >> i) & 0x1);
          } 
        }
      }
      pos = pos + bits_per_char;
    }
    ptr--;
  }

  PROFILE_END;

}  

/*!
 \param vec   Pointer to vector to convert.

 \return Returns pointer to the allocated/coverted string.

 Converts a vector value into a string, allocating the memory for the string in this
 function and returning a pointer to that string.  The type specifies what type of
 value to change vector into.
*/
char* vector_to_string( vector* vec ) { PROFILE(VECTOR_TO_STRING);

  char*        str = NULL;     /* Pointer to allocated string */
  char*        tmp;            /* Pointer to temporary string value */
  int          i;              /* Loop iterator */
  int          str_size;       /* Number of characters needed to hold vector string */
  int          vec_size;       /* Number of characters needed to hold vector value */
  unsigned int group;          /* Number of vector bits to group together for type */
  char         type_char;      /* Character type specifier */
  int          pos;            /* Current bit position in string */
  nibble       value;          /* Current value of string character */
  char         width_str[20];  /* Holds value of width string to calculate string size */

  if( vec->suppl.part.base == QSTRING ) {

    vec_size  = ((vec->width % 8) == 0) ? ((vec->width / 8) + 1)
                                        : ((vec->width / 8) + 2);
    str   = (char*)malloc_safe( vec_size );
    value = 0;
    pos   = 0;

    for( i=(vec->width - 1); i>=0; i-- ) {
      switch( vec->value[i].part.val.value ) {
        case 0 :  value  = value;           break;
        /*@-shiftnegative@*/
        case 1 :  value |= (1 << (i % 8));  break;
        /*@=shiftnegative@*/
        default:  break;
      }
      assert( pos < vec_size );
      if( (i % 8) == 0 ) {
        str[pos] = value;
        pos++; 
        value    = 0;
      }
    }

    str[pos] = '\0';
 
  } else {

    unsigned int rv;

    switch( vec->suppl.part.base ) {
      case BINARY :  
        vec_size  = (vec->width + 1);
        group     = 1;
        type_char = 'b';
        break;
      case OCTAL :  
        vec_size  = ((vec->width % 3) == 0) ? ((vec->width / 3) + 1)
                                            : ((vec->width / 3) + 2);
        group     = 3;
        type_char = 'o';
        break;
      case HEXIDECIMAL :  
        vec_size  = ((vec->width % 4) == 0) ? ((vec->width / 4) + 1)
                                            : ((vec->width / 4) + 2);
        group     = 4;
        type_char = 'h';
        break;
      default          :  
        print_output( "Internal Error:  Unknown vector_to_string type\n", FATAL, __FILE__, __LINE__ );
        exit( EXIT_FAILURE );
        /*@-unreachable@*/
        break;
        /*@=unreachable@*/
    }

    tmp   = (char*)malloc_safe( vec_size );
    value = 0;
    pos   = 0;

    for( i=(vec->width - 1); i>=0; i-- ) {
      switch( vec->value[i].part.val.value ) {
        case 0 :  value = value;                                              break;
        case 1 :  value = (value < 16) ? ((1 << ((unsigned)i%group)) | value) : value;  break;
        case 2 :  value = 16;                                                 break;
        case 3 :  value = 17;                                                 break;
        default:  break;
      }
      assert( pos < vec_size );
      if( ((unsigned)i % group) == 0 ) {
        switch( value ) {
          case 0x0 :  if( (pos > 0) || (i == 0) ) { tmp[pos] = '0';  pos++; }  break;
          case 0x1 :  tmp[pos] = '1';  pos++;  break;
          case 0x2 :  tmp[pos] = '2';  pos++;  break;
          case 0x3 :  tmp[pos] = '3';  pos++;  break;
          case 0x4 :  tmp[pos] = '4';  pos++;  break;
          case 0x5 :  tmp[pos] = '5';  pos++;  break;
          case 0x6 :  tmp[pos] = '6';  pos++;  break;
          case 0x7 :  tmp[pos] = '7';  pos++;  break;
          case 0x8 :  tmp[pos] = '8';  pos++;  break;
          case 0x9 :  tmp[pos] = '9';  pos++;  break;
          case 0xa :  tmp[pos] = 'A';  pos++;  break;
          case 0xb :  tmp[pos] = 'B';  pos++;  break;
          case 0xc :  tmp[pos] = 'C';  pos++;  break;
          case 0xd :  tmp[pos] = 'D';  pos++;  break;
          case 0xe :  tmp[pos] = 'E';  pos++;  break;
          case 0xf :  tmp[pos] = 'F';  pos++;  break;
          case 16  :  tmp[pos] = 'X';  pos++;  break;
          case 17  :  tmp[pos] = 'Z';  pos++;  break;
          default  :  
            print_output( "Internal Error:  Value in vector_to_string exceeds allowed limit\n", FATAL, __FILE__, __LINE__ );
            exit( EXIT_FAILURE );
            /*@-unreachable@*/
            break;
            /*@=unreachable@*/
        }
        value = 0;
      }
    }

    tmp[pos] = '\0';

    rv = snprintf( width_str, 20, "%d", vec->width );
    assert( rv < 20 );
    str_size = strlen( width_str ) + 2 + strlen( tmp ) + 1 + vec->suppl.part.is_signed;
    str      = (char*)malloc_safe( str_size );
    if( vec->suppl.part.is_signed == 0 ) {
      rv = snprintf( str, str_size, "%d'%c%s", vec->width, type_char, tmp );
    } else {
      rv = snprintf( str, str_size, "%d's%c%s", vec->width, type_char, tmp );
    }
    assert( rv < str_size );

    free_safe( tmp );

  }

  PROFILE_END;

  return( str );

}

/*!
 \param str     String version of value.
 \param quoted  If TRUE, treat the string as a literal.
 
 \return Returns pointer to newly created vector holding string value.

 Converts a string value from the lexer into a vector structure appropriately
 sized.  If the string value size exceeds Covered's maximum bit allowance, return
 a value of NULL to indicate this to the calling function.
*/
vector* vector_from_string( char** str, bool quoted ) { PROFILE(VECTOR_FROM_STRING);

  vector*      vec;                   /* Temporary holder for newly created vector */
  int          bits_per_char;         /* Number of bits represented by a single character in the value string str */
  int          size;                  /* Specifies bit width of vector to create */
  char         value[MAX_BIT_WIDTH];  /* String to store string value in */
  char         stype[2];              /* Temporary holder for type of string being parsed */
  nibble       type;                  /* Type of string being parsed */
  int          chars_read;            /* Number of characters read by a sscanf() function call */
  int          i;                     /* Loop iterator */
  unsigned int j;                     /* Loop iterator */
  int          pos;                   /* Bit position */

  if( quoted ) {

    size = strlen( *str ) * 8;

    /* If we have exceeded the maximum number of bits, return a value of NULL */
    if( size > MAX_BIT_WIDTH ) {

      vec = NULL;

    } else {

      /* Create vector */
      vec = vector_create( size, VTYPE_VAL, TRUE );

      vec->suppl.part.base = QSTRING;
      pos                  = 0;

      for( i=(strlen( *str ) - 1); i>=0; i-- ) {
        for( j=0; j<8; j++ ) {
          vec->value[pos].part.val.value = ((nibble)((*str)[i]) >> j) & 0x1;
          pos++;
        }
      }

    }

  } else {

    if( sscanf( *str, "%d'%[sSdD]%[0-9]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 10;
      type          = DECIMAL;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%d'%[sSbB]%[01xXzZ_\?]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 1;
      type          = BINARY;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%d'%[sSoO]%[0-7xXzZ_\?]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 3;
      type          = OCTAL;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%d'%[sShH]%[0-9a-fA-FxXzZ_\?]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 4;
      type          = HEXIDECIMAL;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sSdD]%[0-9]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 10;
      type          = DECIMAL;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sSbB]%[01xXzZ_\?]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 1;
      type          = BINARY;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sSoO]%[0-7xXzZ_\?]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 3;
      type          = OCTAL;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sShH]%[0-9a-fA-FxXzZ_\?]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 4;
      type          = HEXIDECIMAL;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%[0-9_]%n", value, &chars_read ) == 1 ) {
      bits_per_char = 10;
      type          = DECIMAL;
      stype[0]      = 's';       
      stype[1]      = '\0';
      size          = 32;
      *str          = *str + chars_read;
    } else {
      /* If the specified string is none of the above, return NULL */
      bits_per_char = 0;
    }

    /* If we have exceeded the maximum number of bits, return a value of NULL */
    if( (size > MAX_BIT_WIDTH) || (bits_per_char == 0) ) {

      vec = NULL;

    } else {

      /* Create vector */
      vec = vector_create( size, VTYPE_VAL, TRUE );
      vec->suppl.part.base = type;
      if( type == DECIMAL ) {
        vector_from_int( vec, ato32( value ) );
      } else {
        vector_set_static( vec, value, bits_per_char ); 
      }

      /* Set the signed bit to the appropriate value based on the signed indicator in the vector string */
      if( (stype[0] == 's') || (stype [0] == 'S') ) {
        vec->suppl.part.is_signed = 1;
      } else {
        vec->suppl.part.is_signed = 0;
      }

    }

  }

  PROFILE_END;

  return( vec );

}

/*!
 \param vec    Pointer to vector to set value to.
 \param value  String version of VCD value.
 \param msb    Most significant bit to assign to.
 \param lsb    Least significant bit to assign to.

 \return Returns TRUE if assigned value differs from the original value; otherwise,
         returns FALSE.

 Iterates through specified value string, setting the specified vector value to
 this value.  Performs a VCD-specific bit-fill if the value size is not the size
 of the vector.  The specified value string is assumed to be in binary format.
*/
bool vector_vcd_assign( vector* vec, char* value, int msb, int lsb ) { PROFILE(VECTOR_VCD_ASSIGN);

  bool     retval = FALSE;  /* Return value for this function */
  char*    ptr;             /* Pointer to current character under evaluation */
  int      i;               /* Loop iterator */
  vec_data vval;            /* Temporary vector value holder */

  assert( vec != NULL );
  assert( value != NULL );
  assert( msb <= vec->width );

  /* Set pointer to LSB */
  ptr = (value + strlen( value )) - 1;
  i   = (lsb > 0) ? lsb : 0;
  msb = (lsb > 0) ? msb : msb;
    
  while( ptr >= value ) {

    switch( *ptr ) {
      case '0':  vval.all = 0;  break;
      case '1':  vval.all = 1;  break;
      case 'x':  vval.all = 2;  break;
      case 'z':  vval.all = 3;  break;
      default :  
        {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "VCD file contains value change character that is not four-state" );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          exit( EXIT_FAILURE );
        }
        /*@-unreachable@*/
        break;
        /*@=unreachable@*/
    }

    retval |= vector_set_value( vec, &vval, 1, 0, i );

    ptr--;
    i++;

  }

  ptr++;

  /* Perform VCD value bit-fill if specified value did not match width of vector value */
  for( ; i<=msb; i++ ) {
    if( vval.all == 1 ) { vval.all = 0; }
    retval |= vector_set_value( vec, &vval, 1, 0, i );
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for operation results to be stored.
 \param src1   Source vector 1 to perform operation on.
 \param src2   Source vector 2 to perform operation on.
 \param optab  16-entry operation table.

 \return Returns TRUE if assigned value differs from original vector value; otherwise,
         returns FALSE.

 Generic function that takes in two vectors and performs a bitwise
 operation by using the specified operation table.  The operation
 table consists of an array of 16 integers where the integer values
 range from 0 to 3 (0=0, 1=1, 2=x, 3=z).  src1 will be left shifted
 by 2 and added to the value of src2 to obtain an index to the operation
 table.  The value specified at that location will be assigned to the
 corresponding bit location of the target vector.  Vector sizes will
 be properly compensated by placing zeroes.
*/
bool vector_bitwise_op( vector* tgt, vector* src1, vector* src2, nibble* optab ) { PROFILE(VECTOR_BITWISE_OP);

  bool     retval = FALSE;  /* Return value for this function */
  vector   vec;             /* Temporary vector value */
  vec_data vecval;          /* Temporary nibble value for vector */
  int      i;               /* Loop iterator */
  nibble   bit1;            /* Current bit value for src1 */
  nibble   bit2;            /* Current bit value for src2 */

  vector_init( &vec, &vecval, FALSE, 1, VTYPE_VAL );

  for( i=0; i<tgt->width; i++ ) {

    if( src1->width > i ) {
      bit1 = src1->value[i].part.val.value;
    } else {
      bit1 = 0;
    }

    if( src2->width > i ) {
      bit2 = src2->value[i].part.val.value;
    } else {
      bit2 = 0;
    }

    vec.value[0].part.val.value = optab[ ((bit1 << 2) | bit2) ];
    retval |= vector_set_value( tgt, vec.value, 1, 0, i );
    
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression on left of less than sign.
 \param right  Expression on right of less than sign.
 \param comp_type  Comparison type (0=LT, 1=GT, 2=EQ, 3=CEQ)

 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a bitwise comparison (starting at most significant bit) of the
 left and right expressions.
*/
bool vector_op_compare( vector* tgt, vector* left, vector* right, int comp_type ) { PROFILE(VECTOR_OP_COMPARE);

  bool     retval = FALSE;  /* Return value for this function */
  int      pos;             /* Loop iterator */
  nibble   lbit   = 0;      /* Current left expression bit value */
  nibble   rbit   = 0;      /* Current right expression bit value */
  nibble   tmp;             /* Temporary nibble holder */
  bool     done   = FALSE;  /* Specifies continuation of comparison */
  vec_data value;           /* Result to be stored in tgt */
  bool     is_signed;       /* Specifies if we are doing a signed compare */

  /* Determine at which bit position to begin comparing, start at MSB of largest vector */
  if( left->width > right->width ) {
    pos = left->width - 1;
  } else {
    pos = right->width - 1;
  }

  /* Calculate if we are doing a signed compare */
  is_signed = (left->suppl.part.is_signed == 1) && (right->suppl.part.is_signed == 1);

  /* Initialize lbit/rbit values */
  if( left->value[left->width-1].part.val.value < 2 ) {
    lbit = is_signed ? left->value[left->width-1].part.val.value : 0;
  } else {
    lbit = 2;
  }
  if( right->value[right->width-1].part.val.value < 2 ) {
    rbit = is_signed ? right->value[right->width-1].part.val.value : 0;
  } else {
    rbit = 2;
  }

  /* If we are signed and the MSBs are different values, don't go further and reverse the lbit/rbit values */
  if( is_signed && (lbit != rbit) ) {
    done = TRUE;
    tmp  = lbit;
    lbit = rbit;
    rbit = tmp;
  }

  while( (pos >= 0) && !done ) {

    if( pos < left->width ) {
      lbit = left->value[pos].part.val.value;
    }

    if( pos < right->width ) {
      rbit = right->value[pos].part.val.value;
    }

    if( comp_type == COMP_CXEQ ) {
      if( (lbit < 2) && (rbit < 2) && (lbit != rbit) ) {
        done = TRUE;
      }
    } else if( comp_type == COMP_CZEQ ) {
      if( (lbit < 3) && (rbit < 3) && (lbit != rbit) ) {
        done = TRUE;
      }
    } else {
      if( (lbit != rbit) || (((lbit >= 2) || (rbit >= 2)) && (comp_type != COMP_CEQ) && (comp_type != COMP_CNE)) ) {
        done = TRUE;
      }
    }

    pos--;

  }

  if( ((lbit >= 2) || (rbit >= 2)) && 
      (comp_type != COMP_CEQ)      && 
      (comp_type != COMP_CNE)      &&
      (comp_type != COMP_CXEQ)     &&
      (comp_type != COMP_CZEQ) ) {

    value.all = 2;

  } else {

    switch( comp_type ) {
      case COMP_LT   :  value.all = ((lbit == 0) && (rbit == 1))                   ? 1 : 0;  break;
      case COMP_GT   :  value.all = ((lbit == 1) && (rbit == 0))                   ? 1 : 0;  break;
      case COMP_LE   :  value.all = ((lbit == 1) && (rbit == 0))                   ? 0 : 1;  break;
      case COMP_GE   :  value.all = ((lbit == 0) && (rbit == 1))                   ? 0 : 1;  break;
      case COMP_EQ   :
      case COMP_CEQ  :  value.all = (lbit == rbit)                                 ? 1 : 0;  break;
      case COMP_CXEQ :  value.all = ((lbit == rbit) || (lbit >= 2) || (rbit >= 2)) ? 1 : 0;  break;
      case COMP_CZEQ :  value.all = ((lbit == rbit) || (lbit >= 3) || (rbit >= 3)) ? 1 : 0;  break;
      case COMP_NE   :
      case COMP_CNE  :  value.all = (lbit == rbit)                                 ? 0 : 1;  break;
      default        :
        {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  Unidentified comparison type %d", comp_type );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          exit( EXIT_FAILURE );
        }
        /*@-unreachable@*/
        break;
        /*@=unreachable@*/
    }

  }

  retval = vector_set_value( tgt, &value, 1, 0, 0 );

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value being shifted left.
 \param right  Expression containing number of bit positions to shift.

 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Converts right expression into an integer value and left shifts the left
 expression the specified number of bit locations, zero-filling the LSB.
*/
bool vector_op_lshift( vector* tgt, vector* left, vector* right ) { PROFILE(VECTOR_OP_LSHIFT);

  bool     retval  = FALSE;  /* Return value for this function */
  int      shift_val;        /* Number of bits to shift left */
  vec_data zero;             /* Zero value for zero-fill */
  vec_data unknown;          /* X-value for unknown fill */
  int      i;                /* Loop iterator */

  zero.all    = 0;
  unknown.all = 2;

  if( vector_is_unknown( right ) ) {

    for( i=0; i<tgt->width; i++ ) {
      retval |= vector_set_value( tgt, &unknown, 1, 0, i );
    }

  } else {

    /* Zero-fill LSBs */
    for( i=0; i<tgt->width; i++ ) {
      retval |= vector_set_value( tgt, &zero, 1, 0, i );
    }

    shift_val = vector_to_int( right );

    if( shift_val < left->width ) {
      retval |= vector_set_value( tgt, left->value, (left->width - shift_val), 0, shift_val );
    }

  }

  PROFILE_END;

  return( retval );
    
}
 
/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value being shifted left.
 \param right  Expression containing number of bit positions to shift.

 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Converts right expression into an integer value and right shifts the left
 expression the specified number of bit locations, zero-filling the MSB.
*/
bool vector_op_rshift( vector* tgt, vector* left, vector* right ) { PROFILE(VECTOR_OP_RSHIFT);

  bool     retval = FALSE;  /* Return value for this function */
  int      shift_val;       /* Number of bits to shift left */
  vec_data zero;            /* Zero value for zero-fill */
  vec_data unknown;         /* X-value for unknown fill */
  int      i;               /* Loop iterator */

  zero.all    = 0;
  unknown.all = 2;

  if( vector_is_unknown( right ) ) {

    for( i=0; i<tgt->width; i++ ) {
      retval |= vector_set_value( tgt, &unknown, 1, 0, i );
    }

  } else {

    /* Perform zero-fill */
    for( i=0; i<tgt->width; i++ ) {
      retval |= vector_set_value( tgt, &zero, 1, 0, i );
    }

    shift_val = vector_to_int( right );

    if( shift_val < left->width ) {
      retval |= vector_set_value( tgt, left->value, (left->width - shift_val), shift_val, 0 );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value being shifted left.
 \param right  Expression containing number of bit positions to shift.

 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Converts right expression into an integer value and right shifts the left
 expression the specified number of bit locations, sign extending the MSB.
*/
bool vector_op_arshift( vector* tgt, vector* left, vector* right ) { PROFILE(VECTOR_OP_ARSHIFT);

  bool     retval = FALSE;  /* Return value for this function */
  int      shift_val;       /* Number of bits to shift left */
  vec_data sign;            /* Sign extended value for zero-fill */
  vec_data unknown;         /* X-value for unknown fill */
  int      i;               /* Loop iterator */

  sign.all            = 0;
  sign.part.val.value = left->value[left->width - 1].part.val.value;
  unknown.all         = 2;

  if( vector_is_unknown( right ) ) {

    for( i=0; i<tgt->width; i++ ) {
      retval |= vector_set_value( tgt, &unknown, 1, 0, i );
    }

  } else {

    /* Perform sign extend-fill */
    for( i=0; i<tgt->width; i++ ) {
      retval |= vector_set_value( tgt, &sign, 1, 0, i );
    }

    shift_val = vector_to_int( right );

    if( shift_val < left->width ) {
      retval |= vector_set_value( tgt, left->value, (left->width - shift_val), shift_val, 0 );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value on left side of + sign.
 \param right  Expression value on right side of + sign.

 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs 4-state bitwise addition on left and right expression values.
 Carry bit is discarded (value is wrapped around).
*/
bool vector_op_add( vector* tgt, vector* left, vector* right ) { PROFILE(VECTOR_OP_ADD);

  int    i;               /* Loop iterator */
  int    tgt_width = tgt->width;
  nibble v2st      = tgt->suppl.part.is_2state;
  nibble carry     = 0;

  switch( tgt->suppl.part.type ) {
    case VTYPE_EXP :
      for( i=0; i<tgt_width; i++ ) {
        nibble lbit = (i < left->width)  ? left->value[i].part.val.value  : 0;
        nibble rbit = (i < right->width) ? right->value[i].part.val.value : 0;
        if( (carry > 1) || (lbit > 1) || (rbit > 1) ) {
          tgt->value[i].part.exp.value = v2st ? 0 : 2;
          carry                        = v2st ? 0 : 2;
        } else {
          nibble val = carry + lbit + rbit;
          tgt->value[i].part.exp.value = val & 0x1;
          carry                        = val >> 1;
        }
        tgt->value[i].part.exp.set = 1;
      }
      break;
    case VTYPE_SIG :
      for( i=0; i<tgt_width; i++ ) {
        nibble lbit = (i < left->width)  ? left->value[i].part.val.value  : 0;
        nibble rbit = (i < right->width) ? right->value[i].part.val.value : 0;
        if( (carry > 1) || (lbit > 1) || (rbit > 1) ) {
          tgt->value[i].part.sig.value = v2st ? 0 : 2;
          carry                        = v2st ? 0 : 2;
        } else {
          nibble val = carry + lbit + rbit;
          if( tgt->value[i].part.sig.set ) {
            if( (tgt->value[i].part.sig.value == 0) && ((val & 0x1) == 1) ) {
              tgt->value[i].part.sig.tog01 = 1;
            } else if ( (tgt->value[i].part.sig.value == 1) && ((val & 0x1) == 0) ) {
              tgt->value[i].part.sig.tog10 = 1;
            }
          }
          tgt->value[i].part.sig.value = val & 0x1;
          carry                        = val >> 1;
        }
        tgt->value[i].part.sig.set = 1;
      }
      break;
    case VTYPE_VAL :
      for( i=0; i<tgt_width; i++ ) { 
        nibble lbit = (i < left->width)  ? left->value[i].part.val.value  : 0;
        nibble rbit = (i < right->width) ? right->value[i].part.val.value : 0;
        if( (carry > 1) || (lbit > 1) || (rbit > 1) ) {
          tgt->value[i].part.val.value = v2st ? 0 : 2;
          carry                        = v2st ? 0 : 2;
        } else {
          nibble val = carry + lbit + rbit;
          tgt->value[i].part.val.value = val & 0x1;
          carry                        = val >> 1;
        }
      }
      break;
    case VTYPE_MEM :
      for( i=0; i<tgt_width; i++ ) {
        nibble lbit = (i < left->width)  ? left->value[i].part.val.value  : 0;
        nibble rbit = (i < right->width) ? right->value[i].part.val.value : 0;
        if( (carry > 1) || (lbit > 1) || (rbit > 1) ) {
          tgt->value[i].part.mem.value = v2st ? 0 : 2;
          carry                        = v2st ? 0 : 2;
        } else {
          nibble val = carry + lbit + rbit;
          if( (tgt->value[i].part.sig.value == 0) && ((val & 0x1) == 1) ) {
            tgt->value[i].part.mem.tog01 = 1;
          } else if ( (tgt->value[i].part.sig.value == 1) && ((val & 0x1) == 0) ) {
            tgt->value[i].part.mem.tog10 = 1;
          }
          tgt->value[i].part.mem.value = val & 0x1;
          carry                        = val >> 1;
        }
        tgt->value[i].part.mem.wr = 1;
      }
      break;
  }

  PROFILE_END;

  return( tgt_width > 0 );

}

/*!
 \param tgt  Pointer to vector that will be assigned the new value.
 \param src  Pointer to vector that will be negated.

 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs a twos complement of the src vector and stores the result in the tgt vector.
*/
bool vector_op_negate( vector* tgt, vector* src ) { PROFILE(VECTOR_OP_NEGATE);

  bool    retval = FALSE;  /* Return value for this function */
  vector* vec1;            /* Temporary vector holder */
  vector* vec2;            /* Temporary vector holder */

  /* Create temp vectors */
  vec1 = vector_create( src->width, VTYPE_VAL, TRUE );
  vec2 = vector_create( tgt->width, VTYPE_VAL, TRUE );

  /* Create vector with a value of 1 */
  vec2->value[0].part.val.value = 1;

  /* Perform twos complement inversion on right expression */
  (void)vector_unary_inv( vec1, src );

  /* Add one to the inverted value */
  retval = vector_op_add( tgt, vec1, vec2 );

  /* Deallocate vectors */
  vector_dealloc( vec1 );
  vector_dealloc( vec2 );

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value on left side of - sign.
 \param right  Expression value on right side of - sign.

 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs 4-state bitwise subtraction by performing bitwise inversion
 of right expression value, adding one to the result and adding this
 result to the left expression value.
*/
bool vector_op_subtract( vector* tgt, vector* left, vector* right ) { PROFILE(VECTOR_OP_SUBTRACT);

  bool    retval = FALSE;  /* Return value for this function */
  vector* vec;             /* Temporary vector holder */

  /* Create temp vectors */
  vec = vector_create( tgt->width, VTYPE_VAL, TRUE );

  /* Negate the value on the right */
  (void)vector_op_negate( vec, right );

  /* Add new value to left value */
  retval = vector_op_add( tgt, left, vec );

  /* Deallocate used memory */ 
  vector_dealloc( vec );

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value on left side of * sign.
 \param right  Expression value on right side of * sign.

 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs 4-state conversion multiplication.  If both values
 are known (non-X, non-Z), vectors are converted to integers, multiplication
 occurs and values are converted back into vectors.  If one of the values
 is equal to zero, the value is 0.  If one of the values is equal to one,
 the value is that of the other vector.
*/
bool vector_op_multiply( vector* tgt, vector* left, vector* right ) { PROFILE(VECTOR_OP_MULTIPLY);

  bool     retval = FALSE;  /* Return value for this function */
  vector   lcomp;           /* Compare vector left */
  vec_data lcomp_val;       /* Compare value left */
  vector   rcomp;           /* Compare vector right */
  vec_data rcomp_val;       /* Compare value right */
  vector   vec;             /* Intermediate vector */
  vec_data vec_val[32];     /* Intermediate value */
  int      i;               /* Loop iterator */

  /* Initialize temporary vectors */
  vector_init( &lcomp, &lcomp_val, FALSE, 1,  VTYPE_VAL );
  vector_init( &rcomp, &rcomp_val, FALSE, 1,  VTYPE_VAL );
  vector_init( &vec,   vec_val,    FALSE, 32, VTYPE_VAL );

  (void)vector_unary_op( &lcomp, left,  xor_optab );
  (void)vector_unary_op( &rcomp, right, xor_optab );

  /* Perform 4-state multiplication */
  if( (lcomp.value[0].part.val.value == 2) && (rcomp.value[0].part.val.value == 2) ) {

    for( i=0; i<vec.width; i++ ) {
      vec.value[i].part.val.value = 2;
    }

  } else if( (lcomp.value[0].part.val.value != 2) && (rcomp.value[0].part.val.value== 2) ) {

    if( vector_to_int( left ) == 0 ) {
      vector_from_int( &vec, 0 );
    } else if( vector_to_int( left ) == 1 ) {
      (void)vector_set_value( &vec, right->value, right->width, 0, 0 );
    } else {
      for( i=0; i<vec.width; i++ ) {
        vec.value[i].part.val.value = 2;
      }
    }

  } else if( (lcomp.value[0].part.val.value == 2) && (rcomp.value[0].part.val.value != 2) ) {

    if( vector_to_int( right ) == 0 ) {
      vector_from_int( &vec, 0 );
    } else if( vector_to_int( right ) == 1 ) {
      (void)vector_set_value( &vec, left->value, left->width, 0, 0 );
    } else {
      for( i=0; i<vec.width; i++ ) {
        vec.value[i].part.val.value = 2;
      }
    }

  } else {

    vector_from_int( &vec, (vector_to_int( left ) * vector_to_int( right )) );

  }

  /* Set target value */
  retval = vector_set_value( tgt, vec.value, vec.width, 0, 0 );

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt  Target vector to assign data to

 \return Returns TRUE (increments will always cause a value change)

 Performs an increment operation on the specified vector.
*/
bool vector_op_inc( vector* tgt ) { PROFILE(VECTOR_OP_INC);

  vector* tmp1;  /* Pointer to temporary vector containing the same contents as the target */
  vector* tmp2;  /* Pointer to temporary vector containing the value of 1 */

  /* Get a copy of the vector data */
  vector_copy( tgt, &tmp1 );

  /* Create a vector containing the value of 1 */
  tmp2 = vector_create( tgt->width, VTYPE_VAL, TRUE );
  tmp2->value[0].part.val.value = 1;
  
  /* Finally add the values and assign them back to the target */
  (void)vector_op_add( tgt, tmp1, tmp2 );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param tgt  Target vector to assign data to

 \return Returns TRUE (decrements will always cause a value change)

 Performs an decrement operation on the specified vector.
*/
bool vector_op_dec( vector* tgt ) { PROFILE(VECTOR_OP_DEC);

  vector* tmp1;  /* Pointer to temporary vector containing the same contents as the target */
  vector* tmp2;  /* Pointer to temporary vector containing the value of 1 */

  /* Get a copy of the vector data */
  vector_copy( tgt, &tmp1 );

  /* Create a vector containing the value of 1 */
  tmp2 = vector_create( tgt->width, VTYPE_VAL, TRUE );
  tmp2->value[0].part.val.value = 1;

  /* Finally add the values and assign them back to the target */
  (void)vector_op_subtract( tgt, tmp1, tmp2 );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param tgt  Target vector for operation results to be stored.
 \param src  Source vector to perform operation on.

 \return Returns TRUE if assigned value differs from orignal; otherwise, returns FALSE.

 Performs a bitwise inversion on the specified vector.
*/
bool vector_unary_inv( vector* tgt, vector* src ) { PROFILE(VECTOR_UNARY_INV);

  bool     retval = FALSE;  /* Return value for this function */
  nibble   bit;             /* Selected bit from source vector */
  vector   vec;             /* Temporary vector value */
  vec_data vec_val;         /* Temporary value */
  int      i;               /* Loop iterator */
  int      swidth;          /* Smallest width between tgt and src */

  vector_init( &vec, &vec_val, FALSE, 1, VTYPE_VAL );

  swidth = (tgt->width < src->width) ? tgt->width : src->width;

  for( i=0; i<swidth; i++ ) {

    bit = src->value[i].part.val.value;

    switch( bit ) {
      case 0  :  vec.value[0].part.val.value = 1;  break;
      case 1  :  vec.value[0].part.val.value = 0;  break;
      default :  vec.value[0].part.val.value = 2;  break;
    }
    retval |= vector_set_value( tgt, vec.value, 1, 0, i );

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt    Target vector for operation result storage.
 \param src    Source vector to be operated on.
 \param optab  Operation table.

 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary operation on specified vector value from specifed
 operation table.
*/
bool vector_unary_op( vector* tgt, vector* src, nibble* optab ) { PROFILE(VECTOR_UNARY_OP);

  bool     retval;   /* Return value for this function */
  nibble   uval;     /* Unary operation value */
  nibble   bit;      /* Current bit under evaluation */
  vector   vec;      /* Temporary vector value */
  vec_data vec_val;  /* Temporary value */
  int      i;        /* Loop iterator */

  if( (src->width == 1) && (optab[16] == 1) ) {

    /* Perform inverse operation if our source width is 1 and we are a NOT operation. */
    retval = vector_unary_inv( tgt, src );

  } else {

    vector_init( &vec, &vec_val, FALSE, 1, VTYPE_VAL );

    assert( src != NULL );
    assert( src->value != NULL );

    uval = src->value[0].part.val.value;

    for( i=1; i<src->width; i++ ) {
      bit  = src->value[i].part.val.value;
      uval = optab[ ((uval << 2) | bit) ]; 
    }

    vec.value[0].part.val.value = uval;
    retval = vector_set_value( tgt, vec.value, 1, 0, 0 );

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param tgt  Target vector for operation result storage.
 \param src  Source vector to be operated on.

 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary logical NOT operation on specified vector value.
*/
bool vector_unary_not( vector* tgt, vector* src ) { PROFILE(VECTOR_UNARY_NOT);

  bool     retval;   /* Return value of this function */
  vector   vec;      /* Temporary vector value */
  vec_data vec_val;  /* Temporary value */

  vector_init( &vec, &vec_val, FALSE, 1, VTYPE_VAL );
  (void)vector_unary_op( &vec, src, or_optab );

  retval = vector_unary_inv( tgt, &vec );

  PROFILE_END;

  return( retval );

}

/*!
 \param vec  Pointer to vector to deallocate memory from.

 Deallocates all heap memory that was initially allocated with the malloc
 routine.
*/
void vector_dealloc( vector* vec ) { PROFILE(VECTOR_DEALLOC);

  if( vec != NULL ) {

    /* Deallocate all vector values */
    if( (vec->value != NULL) && (vec->suppl.part.owns_data == 1) ) {
      free_safe( vec->value );
      vec->value = NULL;
    }

    /* Deallocate vector itself */
    free_safe( vec );

  } else {

    /* Internal error, we should never be trying to deallocate a NULL vector */
    assert( vec != NULL );

  }

  PROFILE_END;

}

/*
 $Log$
 Revision 1.111  2008/01/30 05:51:51  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.110  2008/01/22 03:53:18  phase1geo
 Fixing bug 1876417.  Removing obsolete code in expr.c.

 Revision 1.109  2008/01/16 23:10:34  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.108  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.107  2008/01/15 23:01:15  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.106  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.105  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.104  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.103  2008/01/02 23:48:47  phase1geo
 Removing unnecessary check in vector.c.  Fixing bug 1862769.

 Revision 1.102  2008/01/02 06:00:08  phase1geo
 Updating user documentation to include the CLI and profiling documentation.
 (CLI documentation is not complete at this time).  Fixes bug 1861986.

 Revision 1.101  2007/12/31 23:43:36  phase1geo
 Fixing bug 1858408.  Also fixing issues with vector_op_add functionality.

 Revision 1.100  2007/12/20 05:18:30  phase1geo
 Fixing another regression bug with running in --enable-debug mode and removing unnecessary output.

 Revision 1.99  2007/12/20 04:47:50  phase1geo
 Fixing the last of the regression failures from previous changes.  Removing unnecessary
 output used for debugging.

 Revision 1.98  2007/12/19 22:54:35  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.97  2007/12/19 04:27:52  phase1geo
 More fixes for compiler errors (still more to go).  Checkpointing.

 Revision 1.96  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.95  2007/12/12 23:36:57  phase1geo
 Optimized vector_op_add function significantly.  Other improvements made to
 profiler output.  Attempted to optimize the sim_simulation function although
 it hasn't had the intended effect and delay1.3 is currently failing.  Checkpointing
 for now.

 Revision 1.94  2007/12/12 08:04:16  phase1geo
 Adding more timed functions for profiling purposes.

 Revision 1.93  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.92  2007/12/11 23:19:14  phase1geo
 Fixed compile issues and completed first pass injection of profiling calls.
 Working on ordering the calls from most to least.

 Revision 1.91  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.90  2007/09/18 21:41:54  phase1geo
 Removing inport indicator bit in vector and replacing with owns_data bit
 indicator.  Full regression passes.

 Revision 1.89  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.88  2006/11/22 20:20:01  phase1geo
 Updates to properly support 64-bit time.  Also starting to make changes to
 simulator to support "back simulation" for when the current simulation time
 has advanced out quite a bit of time and the simulator needs to catch up.
 This last feature is not quite working at the moment and regressions are
 currently broken.  Checkpointing.

 Revision 1.87  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.86  2006/10/04 22:04:16  phase1geo
 Updating rest of regressions.  Modified the way we are setting the memory rd
 vector data bit (should optimize the score command just a bit).  Also updated
 quite a bit of memory coverage documentation though I still need to finish
 documenting how to understand the report file for this metric.  Cleaning up
 other things and fixing a few more software bugs from regressions.  Added
 marray2* diagnostics to verify endianness in the unpacked dimension list.

 Revision 1.85  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.84  2006/09/26 22:36:38  phase1geo
 Adding code for memory coverage to GUI and related files.  Lots of work to go
 here so we are checkpointing for the moment.

 Revision 1.83  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.82  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.81  2006/09/20 22:38:10  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.80  2006/09/13 23:05:56  phase1geo
 Continuing from last submission.

 Revision 1.79  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.78  2006/08/20 03:21:00  phase1geo
 Adding support for +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, <<<=, >>>=, ++
 and -- operators.  The op-and-assign operators are currently good for
 simulation and code generation purposes but still need work in the comb.c
 file for proper combinational logic underline and reporting support.  The
 increment and decrement operations should be fully supported with the exception
 of their use in FOR loops (I'm not sure if this is supported by SystemVerilog
 or not yet).  Also started adding support for delayed assignments; however, I
 need to rework this completely as it currently causes segfaults.  Added lots of
 new diagnostics to verify this new functionality and updated regression for
 these changes.  Full IV regression now passes.

 Revision 1.77  2006/07/13 04:35:40  phase1geo
 Fixing problem with unary inversion and logical not.  Updated unary1 failures
 as a result.  Still need to run full regression before considering this fully fixed.

 Revision 1.76  2006/07/12 22:16:18  phase1geo
 Fixing hierarchical referencing for instance arrays.  Also attempted to fix
 a problem found with unary1; however, the generated report coverage information
 does not look correct at this time.  Checkpointing what I have done for now.

 Revision 1.75  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.74  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.73  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.72  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.71  2006/02/02 22:37:41  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.70  2006/01/24 23:24:38  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.69  2006/01/10 05:12:48  phase1geo
 Added arithmetic left and right shift operators.  Added ashift1 diagnostic
 to verify their correct operation.

 Revision 1.68  2005/12/19 23:11:27  phase1geo
 More fixes for memory faults.  Full regression passes.  Errors have now been
 eliminated from regression -- just left-over memory issues remain.

 Revision 1.67  2005/12/16 23:09:15  phase1geo
 More updates to remove memory leaks.  Full regression passes.

 Revision 1.66  2005/12/16 06:25:15  phase1geo
 Fixing lshift/rshift operations to avoid reading unallocated memory.  Updated
 assign1.cdd file.

 Revision 1.65  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.64  2005/11/21 22:21:58  phase1geo
 More regression updates.  Also made some updates to debugging output.

 Revision 1.63  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.62  2005/11/18 05:17:01  phase1geo
 Updating regressions with latest round of changes.  Also added bit-fill capability
 to expression_assign function -- still more changes to come.  We need to fix the
 expression sizing problem for RHS expressions of assignment operators.

 Revision 1.61  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.60  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.59  2005/01/25 13:42:28  phase1geo
 Fixing segmentation fault problem with race condition checking.  Added race1.1
 to regression.  Removed unnecessary output statements from previous debugging
 checkins.

 Revision 1.58  2005/01/11 14:24:16  phase1geo
 Intermediate checkin.

 Revision 1.57  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.56  2005/01/07 23:00:10  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.55  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.54  2005/01/06 23:51:17  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.53  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.52  2004/10/22 22:03:32  phase1geo
 More incremental changes to increase score command efficiency.

 Revision 1.51  2004/10/22 20:31:07  phase1geo
 Returning assignment status in vector_set_value and speeding up assignment procedure.
 This is an incremental change to help speed up design scoring.

 Revision 1.50  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.49  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.48  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.47  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.46  2004/01/28 17:05:17  phase1geo
 Changing toggle report information from binary output format to hexidecimal
 output format for ease in readability for large signal widths.  Updated regression
 for this change and added new toggle.v diagnostic to verify these changes.

 Revision 1.45  2004/01/16 23:05:01  phase1geo
 Removing SET bit from being written to CDD files.  This value is meaningless
 after scoring has completed and sometimes causes miscompares when simulators
 change.  Updated regression for this change.  This change should also be made
 to stable release.

 Revision 1.44  2003/11/12 17:34:03  phase1geo
 Fixing bug where signals are longer than allowable bit width.

 Revision 1.43  2003/11/05 05:22:56  phase1geo
 Final fix for bug 835366.  Full regression passes once again.

 Revision 1.42  2003/10/30 05:05:13  phase1geo
 Partial fix to bug 832730.  This doesn't seem to completely fix the parameter
 case, however.

 Revision 1.41  2003/10/28 13:28:00  phase1geo
 Updates for more FSM attribute handling.  Not quite there yet but full regression
 still passes.

 Revision 1.40  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.39  2003/10/11 05:15:08  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.38  2003/09/15 01:13:57  phase1geo
 Fixing bug in vector_to_int() function when LSB is not 0.  Fixing
 bug in arc_state_to_string() function in creating string version of state.

 Revision 1.37  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.36  2003/08/10 00:05:16  phase1geo
 Fixing bug with posedge, negedge and anyedge expressions such that these expressions
 must be armed before they are able to be evaluated.  Fixing bug in vector compare function
 to cause compare to occur on smallest vector size (rather than on largest).  Updated regression
 files and added new diagnostics to test event fix.

 Revision 1.35  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.34  2003/02/14 00:00:30  phase1geo
 Fixing bug with vector_vcd_assign when signal being assigned has an LSB that
 is greater than 0.

 Revision 1.33  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.32  2003/02/11 05:20:52  phase1geo
 Fixing problems with merging constant/parameter vector values.  Also fixing
 bad output from merge command when the CDD files cannot be opened for reading.

 Revision 1.31  2003/02/10 06:08:56  phase1geo
 Lots of parser updates to properly handle UDPs, escaped identifiers, specify blocks,
 and other various Verilog structures that Covered was not handling correctly.  Fixes
 for proper event type handling.  Covered can now handle most of the IV test suite from
 a parsing perspective.

 Revision 1.30  2003/02/05 22:50:56  phase1geo
 Some minor tweaks to debug output and some minor bug "fixes".  At this point
 regression isn't stable yet.

 Revision 1.29  2003/01/04 03:56:28  phase1geo
 Fixing bug with parameterized modules.  Updated regression suite for changes.

 Revision 1.28  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.27  2002/11/08 00:58:04  phase1geo
 Attempting to fix problem with parameter handling.  Updated some diagnostics
 in test suite.  Other diagnostics to follow.

 Revision 1.26  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.25  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.24  2002/10/31 23:14:32  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.23  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.22  2002/10/24 05:48:58  phase1geo
 Additional fixes for MBIT_SEL.  Changed some philosophical stuff around for
 cleaner code and for correctness.  Added some development documentation for
 expressions and vectors.  At this point, there is a bug in the way that
 parameters are handled as far as scoring purposes are concerned but we no
 longer segfault.

 Revision 1.21  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.20  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.19  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.18  2002/09/19 02:50:02  phase1geo
 Causing previously assigned bit to not get set when value does not change.
 This is necessary to support different Verilog compiler approaches to displaying
 initial values of undefined signals.

 Revision 1.17  2002/09/12 05:16:25  phase1geo
 Updating all CDD files in regression suite due to change in vector handling.
 Modified vectors to assign a default value of 0xaa to unassigned registers
 to eliminate bugs where values never assigned and VCD file doesn't contain
 information for these.  Added initial working version of depth feature in
 report generation.  Updates to man page and parameter documentation.

 Revision 1.16  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.15  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.14  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.13  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.12  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.11  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.10  2002/07/09 17:27:25  phase1geo
 Fixing default case item handling and in the middle of making fixes for
 report outputting.

 Revision 1.9  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.8  2002/07/05 04:35:53  phase1geo
 Adding fixes for casex and casez for proper equality calculations.  casex has
 now been tested and added to regression suite.  Full regression passes.

 Revision 1.6  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

