/*!
 \file     vector.c
 \author   Trevor Williams  (trevorw@charter.net)
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


nibble xor_optab[OPTAB_SIZE]  = { XOR_OP_TABLE  };  /*!< XOR operation table  */
nibble and_optab[OPTAB_SIZE]  = { AND_OP_TABLE  };  /*!< AND operation table  */
nibble or_optab[OPTAB_SIZE]   = { OR_OP_TABLE   };  /*!< OR operation table   */
nibble nand_optab[OPTAB_SIZE] = { NAND_OP_TABLE };  /*!< NAND operation table */
nibble nor_optab[OPTAB_SIZE]  = { NOR_OP_TABLE  };  /*!< NOR operation table  */
nibble nxor_optab[OPTAB_SIZE] = { NXOR_OP_TABLE };  /*!< NXOR operation table */
nibble add_optab[OPTAB_SIZE]  = { ADD_OP_TABLE  };  /*!< ADD operation table  */

extern char user_msg[USER_MSG_LENGTH];


/*!
 \param vec    Pointer to vector to initialize.
 \param value  Pointer to nibble array for vector.
 \param width  Bit width of specified vector.
 
 Initializes the specified vector with the contents of width
 and value (if value != NULL).  If value != NULL, initializes all contents 
 of value array to 0x2 (X-value).
*/
void vector_init( vector* vec, nibble* value, int width ) {

  int i;        /* Loop iterator */

  vec->width = width;
  vec->suppl = 0;
  vec->value = value;

  if( value != NULL ) {

    assert( width > 0 );

    for( i=0; i<width; i++ ) {
      vec->value[i] = 0x0;
    }

  }

}

/*!
 \param width  Bit width of this vector.
 \param data   If FALSE only initializes width but does not allocate a nibble array.

 \return Pointer to newly created vector.

 Creates new vector from heap memory and initializes all vector contents.
*/
vector* vector_create( int width, bool data ) {

  vector* new_vec;       /* Pointer to newly created vector               */
  nibble* value = NULL;  /* Temporarily stores newly created vector value */

  assert( width > 0 );

  new_vec = (vector*)malloc_safe( sizeof( vector ) );

  if( data == TRUE ) {
    value = (nibble*)malloc_safe( sizeof( nibble ) * width );
  }

  vector_init( new_vec, value, width );

  return( new_vec );

}

/*!
 \param from_vec  Vector to copy.
 \param to_vec    Newly created vector copy.
 
 Copies the contents of the from_vec to the to_vec, allocating new memory.
*/
void vector_copy( vector* from_vec, vector** to_vec ) {

  int i;  /* Loop iterator */

  if( from_vec == NULL ) {

    /* If from_vec is NULL, just assign to_vec to NULL */
    *to_vec = NULL;

  } else {

    /* Create vector */
    *to_vec = vector_create( from_vec->width, TRUE );

    /* Copy contents of value array */
    for( i=0; i<from_vec->width; i++ ) {
      (*to_vec)->value[i] = from_vec->value[i];
    }

  }

}

unsigned int vector_nibbles_to_uint( nibble dat0, nibble dat1, nibble dat2, nibble dat3 ) {

  unsigned int d[4];  /* Array of unsigned int format of dat0,1,2,3 */
  int          i;     /* Loop iterator                              */

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

  return( d[0] | d[1] | d[2] | d[3] );

}

void vector_uint_to_nibbles( unsigned int data, nibble* dat ) {

  int i;  /* Loop iterator */

  for( i=0; i<4; i++ ) {
    dat[i] = (nibble)( (((data & (0x00000003 << (i * 2))) >> ((i * 2) + 0)) |
                        ((data & (0x00000100 << i)      ) >> (i +  6)) |
                        ((data & (0x00001000 << i)      ) >> (i +  9)) |
                        ((data & (0x00010000 << i)      ) >> (i + 12)) |
                        ((data & (0x00100000 << i)      ) >> (i + 15)) |
                        ((data & (0x01000000 << i)      ) >> (i + 18)) |
                        ((data & (0x10000000 << i)      ) >> (i + 21))) & 0xff );
  }

}

/*!
 \param vec         Pointer to vector to display to database file.
 \param file        Pointer to coverage database file to display to.
 \param write_data  If set to TRUE, causes 4-state data bytes to be included.

 Writes the specified vector to the specified coverage database file.
*/
void vector_db_write( vector* vec, FILE* file, bool write_data ) {

  int    i;      /* Loop iterator                       */
  nibble mask;   /* Mask value for vector value nibbles */

  mask = write_data ? 0xf : 0xc;

  /* Output vector information to specified file */
  fprintf( file, "%d %d",
    vec->width,
    vec->suppl
  );

  assert( vec->width <= MAX_BIT_WIDTH );

  if( vec->value == NULL ) {

    /* If the vector value was NULL, output default value */
    for( i=0; i<vec->width; i+=4 ) {
      switch( vec->width - i ) {
        case 0 :  break;
        case 1 :  fprintf( file, " %x", vector_nibbles_to_uint( 0x2, 0x0, 0x0, 0x0 ) );  break;
        case 2 :  fprintf( file, " %x", vector_nibbles_to_uint( 0x2, 0x2, 0x0, 0x0 ) );  break;
        case 3 :  fprintf( file, " %x", vector_nibbles_to_uint( 0x2, 0x2, 0x2, 0x0 ) );  break;
        default:  fprintf( file, " %x", vector_nibbles_to_uint( 0x2, 0x2, 0x2, 0x2 ) );  break;
      }
    }

  } else {

    for( i=0; i<vec->width; i+=4 ) {
      switch( vec->width - i ) {
        case 0 :  break;
        case 1 :
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0] & mask) | (write_data ? 0 : 0x2)), 0, 0, 0 ) );
          break;
        case 2 :
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0] & mask) | (write_data ? 0 : 0x2)),
                                                        ((vec->value[i+1] & mask) | (write_data ? 0 : 0x2)), 0, 0 ) );
          break;
        case 3 :
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0] & mask) | (write_data ? 0 : 0x2)), 
                                                        ((vec->value[i+1] & mask) | (write_data ? 0 : 0x2)),
                                                        ((vec->value[i+2] & mask) | (write_data ? 0 : 0x2)), 0 ) );
          break;
        default:
          fprintf( file, " %x", vector_nibbles_to_uint( ((vec->value[i+0] & mask) | (write_data ? 0 : 0x2)), 
                                                        ((vec->value[i+1] & mask) | (write_data ? 0 : 0x2)),
                                                        ((vec->value[i+2] & mask) | (write_data ? 0 : 0x2)),
                                                        ((vec->value[i+3] & mask) | (write_data ? 0 : 0x2)) ) );
          break;
      }
    }

  }

}

/*!
 \param vec    Pointer to vector to create.
 \param line   Pointer to line to parse for vector information.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Creates a new vector structure, parses current file line for vector information
 and returns new vector structure to calling function.
*/
bool vector_db_read( vector** vec, char** line ) {

  bool         retval = TRUE;  /* Return value for this function    */
  int          width;          /* Vector bit width                  */
  int          suppl;          /* Temporary supplemental value      */
  int          i;              /* Loop iterator                     */
  int          chars_read;     /* Number of characters read         */
  unsigned int value;          /* Temporary value                   */
  nibble       nibs[4];        /* Temporary nibble value containers */

  /* Read in vector information */
  if( sscanf( *line, "%d %d%n", &width, &suppl, &chars_read ) == 2 ) {

    *line = *line + chars_read;

    /* Create new vector */
    *vec = vector_create( width, TRUE );
    (*vec)->suppl = (char)suppl & 0xff;

    i = 0;
    while( (i < width) && retval ) {
      if( sscanf( *line, "%x%n", &value, &chars_read ) == 1 ) {
        *line += chars_read;
        vector_uint_to_nibbles( value, nibs );
        switch( width - i ) {
          case 0 :  break;
          case 1 :
            (*vec)->value[i+0] = nibs[0];
            break;
          case 2 :
            (*vec)->value[i+0] = nibs[0];
            (*vec)->value[i+1] = nibs[1];
            break;
          case 3 :
            (*vec)->value[i+0] = nibs[0];
            (*vec)->value[i+1] = nibs[1];
            (*vec)->value[i+2] = nibs[2];
            break;
          default:
            (*vec)->value[i+0] = nibs[0];
            (*vec)->value[i+1] = nibs[1];
            (*vec)->value[i+2] = nibs[2];
            (*vec)->value[i+3] = nibs[3];
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

  return( retval );

}

/*!
 \param base  Base vector to merge data into.
 \param line  Pointer to line to parse for vector information.
 \param same  Specifies if vector to merge needs to be exactly the same as the existing vector.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Parses current file line for vector information and performs vector merge of 
 base vector and read vector information.  If the vectors are found to be different
 (width is not equal), an error message is sent to the user and the
 program is halted.  If the vectors are found to be equivalents, the merge is
 performed on the vector nibbles.
*/
bool vector_db_merge( vector* base, char** line, bool same ) {

  bool   retval = TRUE;   /* Return value of this function */
  int    width;           /* Width of read vector          */
  int    suppl;           /* Supplemental value of vector  */
  int    chars_read;      /* Number of characters read     */
  int    i;               /* Loop iterator                 */
  int    value;           /* Integer form of value         */
  nibble nibs[4];         /* Temporary nibble containers   */     

  assert( base != NULL );

  if( sscanf( *line, "%d %d%n", &width, &suppl, &chars_read ) == 2 ) {

    *line = *line + chars_read;

    if( base->width != width ) {

      if( same ) {
        print_output( "Attempting to merge databases derived from different designs.  Unable to merge", FATAL );
        exit( 1 );
      }

    } else {

      i = 0;
      while( (i < width) && retval ) {
        if( sscanf( *line, "%x%n", &value, &chars_read ) == 1 ) {
          *line += chars_read;
          vector_uint_to_nibbles( value, nibs );
          switch( width - i ) {
            case 0 :  break;
            case 1 :  
              base->value[i+0] = (base->value[i+0] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              break;
            case 2 :
              base->value[i+0] = (base->value[i+0] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              base->value[i+1] = (base->value[i+1] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[1] & VECTOR_MERGE_MASK);
              break;
            case 3 :
              base->value[i+0] = (base->value[i+0] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              base->value[i+1] = (base->value[i+1] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[1] & VECTOR_MERGE_MASK);
              base->value[i+2] = (base->value[i+2] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[2] & VECTOR_MERGE_MASK);
              break;
            default:
              base->value[i+0] = (base->value[i+0] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[0] & VECTOR_MERGE_MASK);
              base->value[i+1] = (base->value[i+1] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[1] & VECTOR_MERGE_MASK);
              base->value[i+2] = (base->value[i+2] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[2] & VECTOR_MERGE_MASK);
              base->value[i+3] = (base->value[i+3] & (VECTOR_MERGE_MASK | 0x3)) | (nibs[3] & VECTOR_MERGE_MASK);
              break;
          }
        } else {
          retval = FALSE;
        }
        i += 4;
      }
                       
    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param nib    Nibble to display toggle information
 \param width  Number of bits of nibble to display
 \param ofile  Stream to output information to.
 
 Displays the toggle01 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle01( nibble* nib, int width, FILE* ofile ) {

  int i;    /* Loop iterator */

  fprintf( ofile, "%d'b", width );

  for( i=(width - 1); i>=0; i-- ) {
    fprintf( ofile, "%d", VECTOR_TOG01( nib[i] ) );
  }

}

/*!
 \param nib    Nibble to display toggle information
 \param width  Number of bits of nibble to display
 \param ofile  Stream to output information to.
 
 Displays the toggle10 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle10( nibble* nib, int width, FILE* ofile ) {

  int i;    /* Loop iterator */

  fprintf( ofile, "%d'b", width );

  for( i=(width - 1); i>=0; i-- ) {
    fprintf( ofile, "%d", VECTOR_TOG10( nib[i] ) );
  }

}

/*!
 \param nib    Nibble to display.
 \param width  Number of bits in nibble to display.

 Outputs the specified nibble array to standard output as described by the
 width parameter.
*/
void vector_display_nibble( nibble* nib, int width ) {

  int i;       /* Loop iterator */

  printf( "\n" );
  printf( "      raw value:" );
  
  for( i=(width - 1); i>=0; i-- ) {
    printf( " %08x", nib[i] );
  }

  /* Display nibble value */
  printf( ", value: %d'b", width );

  for( i=(width - 1); i>=0; i-- ) {
    switch( VECTOR_VAL( nib[i] ) ) {
      case 0 :  printf( "0" );  break;
      case 1 :  printf( "1" );  break;
      case 2 :  printf( "x" );  break;
      case 3 :  printf( "z" );  break;
      default:  break;
    }
  }

  /* Display nibble toggle01 history */
  printf( ", 0->1: " );
  vector_display_toggle01( nib, width, stdout );

  /* Display nibble toggle10 history */
  printf( ", 1->0: " );
  vector_display_toggle10( nib, width, stdout );

  /* Display bit set information */
  printf( ", set: %d'b", width );

  for( i=(width - 1); i>=0; i-- ) {
    printf( "%d", VECTOR_SET( nib[i] ) );
  }

  /* Display bit FALSE information */
  printf( ", FALSE: %d'b", width );

  for( i=(width - 1); i>=0; i-- ) {
    printf( "%d", VECTOR_FALSE( nib[i] ) );
  }

  /* Display bit TRUE information */
  printf( ", TRUE: %d'b", width );

  for( i=(width - 1); i>=0; i-- ) {
    printf( "%d", VECTOR_TRUE( nib[i] ) );
  }

}

/*!
 \param vec  Pointer to vector to output to standard output.

 Outputs contents of vector to standard output (for debugging purposes only).
*/
void vector_display( vector* vec ) {

  assert( vec != NULL );

  printf( "Vector => width: %d, ", vec->width );

  if( (vec->width > 0) && (vec->value != NULL) ) {
    vector_display_nibble( vec->value, vec->width );
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
void vector_toggle_count( vector* vec, int* tog01_cnt, int* tog10_cnt ) {

  int i;  /* Loop iterator */

  for( i=0; i<vec->width; i++ ) {
    *tog01_cnt = *tog01_cnt + VECTOR_TOG01( vec->value[i] );
    *tog10_cnt = *tog10_cnt + VECTOR_TOG10( vec->value[i] );
  }

}

/*!
 \param vec        Pointer to vector to parse.
 \param false_cnt  Number of bits in vector that was set to a value of FALSE.
 \param true_cnt   Number of bits in vector that was set to a value of TRUE.

 Walks through specified vector counting the number of FALSE bits that
 are set and the number of TRUE bits that are set.  Adds these values
 to the contents of false_cnt and true_cnt.
*/
void vector_logic_count( vector* vec, int* false_cnt, int* true_cnt ) {

  int i;  /* Loop iterator */

  for( i=0; i<vec->width; i++ ) {
    *false_cnt = *false_cnt + VECTOR_FALSE( vec->value[i] );
    *true_cnt  = *true_cnt  + VECTOR_TRUE( vec->value[i] );
  }

}

/*!
 \param vec       Pointer to vector to set value to.
 \param value     New value to set vector value to.
 \param width     Width of new value.
 \param from_idx  Starting bit index of value to start copying.
 \param to_idx    Starting bit index of value to copy to.

 \return Returns TRUE if assignment was successful; otherwise, returns FALSE.

 Allows the calling function to set any bit vector within the vector
 range.  If the vector value has never been set, sets
 the value to the new value and returns.  If the vector value has previously
 been set, checks to see if new vector bits have toggled, sets appropriate
 toggle values, sets the new value to this value and returns.
*/
void vector_set_value( vector* vec, nibble* value, int width, int from_idx, int to_idx ) {

  nibble from_val;  /* Current bit value of value being assigned */
  nibble to_val;    /* Current bit value of previous value       */
  int    i;         /* Loop iterator                             */
  nibble set_val;   /* Value to set current vec value to         */

  assert( vec != NULL );

  /* Verify that index is within range */
  /* printf( "to_idx: %d, vec->width: %d\n", to_idx, vec->width ); */
  assert( to_idx < vec->width );
  assert( to_idx >= 0 );

  /* Adjust width to smaller of two values */
  width = (width > (vec->width - to_idx)) ? (vec->width - to_idx) : width;

  for( i=0; i<width; i++ ) {

    set_val  = vec->value[i + to_idx];
    from_val = VECTOR_VAL( value[i + from_idx] );
    to_val   = VECTOR_VAL( set_val );

    if( VECTOR_SET( set_val ) == 0x1 ) {

      /* Assign toggle values if necessary */

      if( (to_val == 0) && (from_val == 1) ) {
        set_val |= (0x1 << VECTOR_LSB_TOG01);
      } else if( (to_val == 1) && (from_val == 0) ) {
        set_val |= (0x1 << VECTOR_LSB_TOG10);
      }

    }

    /* Assign TRUE/FALSE values if necessary */
    if( from_val == 0 ) {
      set_val |= (0x1 << VECTOR_LSB_FALSE);
    } else if( from_val == 1 ) {
      set_val |= (0x1 << VECTOR_LSB_TRUE);
    }

    /* Perform value assignment */
    if( from_val != to_val ) {
      set_val |= (0x1 << VECTOR_LSB_SET);
    }

    VECTOR_SET_VAL( set_val, from_val );

    vec->value[i + to_idx] = set_val;

  }

}

/*!
 \param vec  Pointer to vector to check for unknowns.

 \return Returns TRUE if the specified vector contains unknown values; otherwise, returns FALSE.

 Checks specified vector for any X or Z values and returns TRUE if any are found; otherwise,
 returns a value of false.  This function is useful to be called before vector_to_int is called.
*/
bool vector_is_unknown( vector* vec ) {

  bool unknown = FALSE;  /* Specifies if vector contains unknown values */
  int  i;                /* Loop iterator                               */
  int  val;              /* Bit value of current bit                    */

  for( i=0; i<vec->width; i++ ) {
    val = VECTOR_VAL( vec->value[i] );
    if( (val == 0x2) || (val == 0x3) ) {
      unknown = TRUE;
    }
  }

  return( unknown );

}

/*!
 \param vec  Pointer to vector to convert into integer.

 \return Returns integer value of specified vector.

 Converts a vector structure into an integer value.  If the number of bits for the
 vector exceeds the number of bits in an integer, the upper bits of the vector are
 unused.
*/
int vector_to_int( vector* vec ) {

  int retval = 0;   /* Integer value returned to calling function */
  int i;            /* Loop iterator                              */
  int width;        /* Number of bits to use in creating integer  */

  width = (vec->width > (SIZEOF_INT * 8)) ? 32 : vec->width;

  for( i=(width - 1); i>=0; i-- ) {
    switch( VECTOR_VAL( vec->value[i] ) ) {
      case 0 :  retval = (retval << 1) | 0;  break;
      case 1 :  retval = (retval << 1) | 1;  break;
      default:
        print_output( "Vector converting to integer contains X or Z values", FATAL );
        assert( VECTOR_VAL( vec->value[i] ) < 2 );
        break;
    }
  }

  return( retval );

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
void vector_from_int( vector* vec, int value ) {

  int width;      /* Number of bits to convert */
  int i;          /* Loop iterator             */

  width = (vec->width < (SIZEOF_INT * 8)) ? vec->width : (SIZEOF_INT * 8);

  for( i=0; i<width; i++ ) {
    VECTOR_SET_VAL( vec->value[i], (value & 0x1) );
    value >>= 1;
  }

}

/*!
 \param vec            Pointer to vector to add static value to.
 \param str            Value string to add.
 \param bits_per_char  Number of bits necessary to store a value character (1, 3, or 4).

 Iterates through string str starting at the left-most character, calculates the int value
 of the character and sets the appropriate number of bits in the specified vector locations.
*/
void vector_set_static( vector* vec, char* str, int bits_per_char ) {

  char* ptr;      /* Pointer to current character evaluating         */
  int   pos;      /* Current bit position in vector                  */
  int   val;      /* Temporary holder for value of current character */
  int   i;        /* Loop iterator                                   */

  pos = 0;

  ptr = str + (strlen( str ) - 1);

  while( ptr >= str ) {
    if( *ptr != '_' ) {
      if( (*ptr == 'x') || (*ptr == 'X') ) {
        for( i=0; i<bits_per_char; i++ ) {
          VECTOR_SET_VAL( vec->value[i + pos], 0x2 );
        }
      } else if( (*ptr == 'z') || (*ptr == 'Z') || (*ptr == '?') ) {
        for( i=0; i<bits_per_char; i++ ) {
          VECTOR_SET_VAL( vec->value[i + pos], 0x3 );
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
  	  VECTOR_SET_VAL( vec->value[i + pos], ((val >> i) & 0x1) );
        }
      }
      pos = pos + bits_per_char;
    }
    ptr--;
  }

}  

/*!
 \param vec   Pointer to vector to convert.
 \param type  Specifies the type of string to create (DECIMAL, OCTAL, HEXIDECIMAL, BINARY)

 \return Returns pointer to the allocated/coverted string.

 Converts a vector value into a string, allocating the memory for the string in this
 function and returning a pointer to that string.  The type specifies what type of
 value to change vector into.
*/
char* vector_to_string( vector* vec, int type ) {

  char*  str = NULL;     /* Pointer to allocated string                          */
  char*  tmp;            /* Pointer to temporary string value                    */
  int    i;              /* Loop iterator                                        */
  int    str_size;       /* Number of characters needed to hold vector string    */
  int    vec_size;       /* Number of characters needed to hold vector value     */
  int    group;          /* Number of vector bits to group together for type     */
  char   type_char;      /* Character type specifier                             */
  int    pos;            /* Current bit position in string                       */
  nibble value;          /* Current value of string character                    */
  char   width_str[20];  /* Holds value of width string to calculate string size */

  switch( type ) {
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
      print_output( "Internal Error:  Unknown vector_to_string type\n", FATAL );
      exit( 1 );
      break;
  }

  tmp   = (char*)malloc_safe( vec_size );
  value = 0;
  pos   = 0;

  for( i=(vec->width - 1); i>=0; i-- ) {
    switch( VECTOR_VAL( vec->value[i] ) ) {
      case 0 :  value = value;                                              break;
      case 1 :  value = (value < 16) ? ((1 << (i%group)) | value) : value;  break;
      case 2 :  value = 16;                                                 break;
      case 3 :  value = 17;                                                 break;
      default:  break;
    }
    assert( pos < vec_size );
    if( (i % group) == 0 ) {
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
          print_output( "Internal Error:  Value in vector_to_string exceeds allowed limit\n", FATAL );
          exit( 1 );
          break;
      }
      value = 0;
    }
  }

  tmp[pos] = '\0';

  snprintf( width_str, 20, "%d", vec->width );
  str_size = strlen( width_str ) + 2 + strlen( tmp ) + 1;
  str      = (char*)malloc_safe( str_size );
  snprintf( str, str_size, "%d'%c%s", vec->width, type_char, tmp );

  free_safe( tmp );

  return( str );

}

/*!
 \param str  String version of value.
 
 \return Returns pointer to newly created vector holding string value.

 Converts a string value from the lexer into a vector structure appropriately
 sized.  If the string value size exceeds Covered's maximum bit allowance, return
 a value of NULL to indicate this to the calling function.
*/
vector* vector_from_string( char** str ) {

  vector* vec;                   /* Temporary holder for newly created vector                                */
  int     bits_per_char;         /* Number of bits represented by a single character in the value string str */
  int     size;                  /* Specifies bit width of vector to create                                  */
  char    value[MAX_BIT_WIDTH];  /* String to store string value in                                          */
  char    stype[2];              /* Temporary holder for type of string being parsed                         */
  int     type;                  /* Type of string being parsed                                              */
  int     chars_read;            /* Number of characters read by a sscanf() function call                    */

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
    size          = 32;
    *str          = *str + chars_read;
  } else {
    /* If the specified string is none of the above, return NULL */
    return( NULL );
  }

  /* If we have exceeded the maximum number of bits, return a value of NULL */
  if( size > MAX_BIT_WIDTH ) {

    vec = NULL;

  } else {

    /* Create vector */
    vec = vector_create( size, TRUE );

    vec->suppl = type;

    if( type == DECIMAL ) {
      vector_from_int( vec, atol( value ) );
    } else {
      vector_set_static( vec, value, bits_per_char ); 
    }

  }

  return( vec );

}

/*!
 \param vec    Pointer to vector to set value to.
 \param value  String version of VCD value.
 \param msb    Most significant bit to assign to.
 \param lsb    Least significant bit to assign to.

 Iterates through specified value string, setting the specified vector value to
 this value.  Performs a VCD-specific bit-fill if the value size is not the size
 of the vector.  The specified value string is assumed to be in binary format.
*/
void vector_vcd_assign( vector* vec, char* value, int msb, int lsb ) {

  char*  ptr;   /* Pointer to current character under evaluation */
  int    i;     /* Loop iterator                                 */
  nibble vval;  /* Temporary vector value holder                 */

  assert( vec != NULL );
  assert( value != NULL );
  assert( msb <= vec->width );

  /* Set pointer to LSB */
  ptr = (value + strlen( value )) - 1;
  i   = (lsb > 0) ? lsb : 0;
  msb = (lsb > 0) ? msb : msb;
    
  while( ptr >= value ) {

    switch( *ptr ) {
      case '0':  vval = 0;  break;
      case '1':  vval = 1;  break;
      case 'x':  vval = 2;  break;
      case 'z':  vval = 3;  break;
      default :  
        snprintf( user_msg, USER_MSG_LENGTH, "VCD file contains value change character that is not four-state" );
        print_output( user_msg, FATAL );
        exit( 1 );
        break;
    }

    vector_set_value( vec, &vval, 1, 0, i );

    ptr--;
    i++;

  }

  ptr++;

  /* Perform VCD value bit-fill if specified value did not match width of vector value */
  for( ; i<=msb; i++ ) {
    if( vval == 1 ) { vval = 0; }
    vector_set_value( vec, &vval, 1, 0, i );
  }

}

/*!
 \param tgt    Target vector for operation results to be stored.
 \param src1   Source vector 1 to perform operation on.
 \param src2   Source vector 2 to perform operation on.
 \param optab  16-entry operation table.

 Generic function that takes in two vectors and performs a bitwise
 operation by using the specified operation table.  The operation
 table consists of an array of 16 integers where the integer values
 range from 0 to 3 (0=0, 1=1, 2=x, 3=z).  src1 will be left shifted
 by 2 and added to the value of src2 to obtain an index to the operation
 table.  The value specified at that location will be assigned to the
 corresponding bit location of the target vector.  Vector sizes will
 be properly compensated by placing zeroes.
*/
void vector_bitwise_op( vector* tgt, vector* src1, vector* src2, nibble* optab ) {

  vector  vec;    /* Temporary vector value            */
  nibble  vecval; /* Temporary nibble value for vector */
  int     i;      /* Loop iterator                     */
  nibble  bit1;   /* Current bit value for src1        */
  nibble  bit2;   /* Current bit value for src2        */

  vector_init( &vec, &vecval, 1 );

  for( i=0; i<tgt->width; i++ ) {

    if( src1->width > i ) {
      bit1 = VECTOR_VAL( src1->value[i] );
    } else {
      bit1 = 0;
    }

    if( src2->width > i ) {
      bit2 = VECTOR_VAL( src2->value[i] );
    } else {
      bit2 = 0;
    }

    VECTOR_SET_VAL( vec.value[0], optab[ ((bit1 << 2) | bit2) ] );
    vector_set_value( tgt, vec.value, 1, 0, i );
    
  }

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression on left of less than sign.
 \param right  Expression on right of less than sign.
 \param comp_type  Comparison type (0=LT, 1=GT, 2=EQ, 3=CEQ)

 Performs a bitwise comparison (starting at most significant bit) of the
 left and right expressions.
*/
void vector_op_compare( vector* tgt, vector* left, vector* right, int comp_type ) {

  int    pos;           /* Loop iterator                        */
  nibble lbit = 0;      /* Current left expression bit value    */
  nibble rbit = 0;      /* Current right expression bit value   */
  bool   done = FALSE;  /* Specifies continuation of comparison */
  nibble value;         /* Result to be stored in tgt           */

  /* Determine at which bit position to begin comparing, start at MSB of smallest vector */
  if( left->width > right->width ) {
    pos = right->width - 1;
  } else {
    pos = left->width - 1;
  }

  while( (pos >= 0) && !done ) {

    if( pos < left->width ) {
      lbit = VECTOR_VAL( left->value[pos] );
    } else {
      lbit = 0;
    }

    if( pos < right->width ) {
      rbit = VECTOR_VAL( right->value[pos] );
    } else {
      rbit = 0;
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

    value = 2;

  } else {

    switch( comp_type ) {
      case COMP_LT   :  value = ((lbit == 0) && (rbit == 1))                   ? 1 : 0;  break;
      case COMP_GT   :  value = ((lbit == 1) && (rbit == 0))                   ? 1 : 0;  break;
      case COMP_LE   :  value = ((lbit == 1) && (rbit == 0))                   ? 0 : 1;  break;
      case COMP_GE   :  value = ((lbit == 0) && (rbit == 1))                   ? 0 : 1;  break;
      case COMP_EQ   :
      case COMP_CEQ  :  value = (lbit == rbit)                                 ? 1 : 0;  break;
      case COMP_CXEQ :  value = ((lbit == rbit) || (lbit >= 2) || (rbit >= 2)) ? 1 : 0;  break;
      case COMP_CZEQ :  value = ((lbit == rbit) || (lbit >= 3) || (rbit >= 3)) ? 1 : 0;  break;
      case COMP_NE   :
      case COMP_CNE  :  value = (lbit == rbit)                                 ? 0 : 1;  break;
      default        :
        snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  Unidentified comparison type %d", comp_type );
        print_output( user_msg, FATAL );
        exit( 1 );
        break;
    }

  }

  vector_set_value( tgt, &value, 1, 0, 0 );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value being shifted left.
 \param right  Expression containing number of bit positions to shift.

 Converts right expression into an integer value and left shifts the left
 expression the specified number of bit locations, zero-filling the LSB.
*/
void vector_op_lshift( vector* tgt, vector* left, vector* right ) {

  int    shift_val;    /* Number of bits to shift left */
  nibble zero    = 0;  /* Zero value for zero-fill     */
  nibble unknown = 2;  /* X-value for unknown fill     */
  int    i;            /* Loop iterator                */

  if( vector_is_unknown( right ) ) {

    for( i=0; i<tgt->width; i++ ) {
      vector_set_value( tgt, &unknown, 1, 0, i );
    }

  } else {

    shift_val = vector_to_int( right );

    if( shift_val >= left->width ) {
      shift_val = left->width;
    } else {
      vector_set_value( tgt, left->value, (left->width - shift_val), 0, shift_val );
    }

    /* Zero-fill LSBs */
    for( i=0; i<shift_val; i++ ) {
      vector_set_value( tgt, &zero, 1, 0, i );
    }

  }
    
}
 
/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value being shifted left.
 \param right  Expression containing number of bit positions to shift.

 Converts right expression into an integer value and right shifts the left
 expression the specified number of bit locations, zero-filling the MSB.
*/
void vector_op_rshift( vector* tgt, vector* left, vector* right ) {

  int    shift_val;    /* Number of bits to shift left */
  nibble zero    = 0;  /* Zero value for zero-fill     */
  nibble unknown = 2;  /* X-value for unknown fill     */
  int    i;            /* Loop iterator                */

  if( vector_is_unknown( right ) ) {

    for( i=0; i<tgt->width; i++ ) {
      vector_set_value( tgt, &unknown, 1, 0, i );
    }

  } else {

    shift_val = vector_to_int( right );

    if( shift_val >= left->width ) {
      shift_val = left->width;
    } else {
      vector_set_value( tgt, left->value, (left->width - shift_val), shift_val, 0 );
    }

    /* Zero-fill LSBs */
    for( i=(left->width - shift_val); i<left->width; i++ ) {
      vector_set_value( tgt, &zero, 1, 0, i );
    }

  }

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value on left side of + sign.
 \param right  Expression value on right side of + sign.

 Performs 4-state bitwise addition on left and right expression values.
 Carry bit is discarded (value is wrapped around).
*/
void vector_op_add( vector* tgt, vector* left, vector* right ) {

  nibble lbit;        /* Current left expression bit  */
  nibble rbit;        /* Current right expression bit */
  nibble carry = 0;   /* Carry bit                    */
  nibble value;       /* Current value                */
  int    i;           /* Loop iterator                */

  for( i=0; i<tgt->width; i++ ) {
    
    if( i < left->width ) {
      lbit = VECTOR_VAL( left->value[i] );
    } else {
      lbit = 0;
    }

    if( i < right->width ) {
      rbit = VECTOR_VAL( right->value[i] );
    } else {
      rbit = 0;
    }

    value =  add_optab[ (((add_optab[((carry << 2) | lbit)] & 0x3) << 2) | rbit) ] & 0x3;
    carry = (((add_optab[ ((carry << 2) | lbit) ] >> 2) & 0x3) |
             ((add_optab[ ((carry << 2) | rbit) ] >> 2) & 0x3) |
             ((add_optab[ ((lbit  << 2) | rbit) ] >> 2) & 0x3));
    

    vector_set_value( tgt, &value, 1, 0, i );

  }

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value on left side of - sign.
 \param right  Expression value on right side of - sign.

 Performs 4-state bitwise subtraction by performing bitwise inversion
 of right expression value, adding one to the result and adding this
 result to the left expression value.
*/
void vector_op_subtract( vector* tgt, vector* left, vector* right ) {

  vector* vec1;  /* Temporary vector holder */
  vector* vec2;  /* Temporary vector holder */
  vector* vec3;  /* Temporary vector holder */

  /* Create temp vectors */
  vec1 = vector_create( tgt->width, TRUE );
  vec2 = vector_create( tgt->width, TRUE );
  vec3 = vector_create( tgt->width, TRUE );

  /* Create vector with a value of 1 */
  VECTOR_SET_VAL( vec1->value[0], 1 );

  /* Perform twos complement inversion on right expression */
  vector_unary_inv( vec2, right );

  /* Add one to the inverted value */
  vector_op_add( vec3, vec2, vec1 );  

  /* Add new value to left value */
  vector_op_add( tgt, left, vec3 );

  vector_dealloc( vec1 );
  vector_dealloc( vec2 );
  vector_dealloc( vec3 );

}

/*!
 \param tgt    Target vector for storage of results.
 \param left   Expression value on left side of * sign.
 \param right  Expression value on right side of * sign.

 Performs 4-state conversion multiplication.  If both values
 are known (non-X, non-Z), vectors are converted to integers, multiplication
 occurs and values are converted back into vectors.  If one of the values
 is equal to zero, the value is 0.  If one of the values is equal to one,
 the value is that of the other vector.
*/
void vector_op_multiply( vector* tgt, vector* left, vector* right ) {

  vector lcomp;        /* Compare vector left  */
  nibble lcomp_val;    /* Compare value left   */
  vector rcomp;        /* Compare vector right */
  nibble rcomp_val;    /* Compare value right  */
  vector vec;          /* Intermediate vector  */
  nibble vec_val[32];  /* Intermediate value   */
  int     i;           /* Loop iterator        */

  /* Initialize temporary vectors */
  vector_init( &lcomp, &lcomp_val, 1 );
  vector_init( &rcomp, &rcomp_val, 1 );
  vector_init( &vec,   vec_val,    32 );

  vector_unary_op( &lcomp, left,  xor_optab );
  vector_unary_op( &rcomp, right, xor_optab );

  /* Perform 4-state multiplication */
  if( ((lcomp.value[0] & 0x3) == 2) && ((rcomp.value[0] & 0x3) == 2) ) {

    for( i=0; i<vec.width; i++ ) {
      VECTOR_SET_VAL( vec.value[i], 2 );
    }

  } else if( ((lcomp.value[0] & 0x3) != 2) && ((rcomp.value[0] & 0x3) == 2) ) {

    if( vector_to_int( left ) == 0 ) {
      vector_from_int( &vec, 0 );
    } else if( vector_to_int( left ) == 1 ) {
      vector_set_value( &vec, right->value, right->width, 0, 0 );
    } else {
      for( i=0; i<vec.width; i++ ) {
        VECTOR_SET_VAL( vec.value[i], 2 );
      }
    }

  } else if( ((lcomp.value[0] & 0x3) == 2) && ((rcomp.value[0] & 0x3) != 2) ) {

    if( vector_to_int( right ) == 0 ) {
      vector_from_int( &vec, 0 );
    } else if( vector_to_int( right ) == 1 ) {
      vector_set_value( &vec, left->value, left->width, 0, 0 );
    } else {
      for( i=0; i<vec.width; i++ ) {
        VECTOR_SET_VAL( vec.value[i], 2 );
      }
    }

  } else {

    vector_from_int( &vec, (vector_to_int( left ) * vector_to_int( right )) );

  }

  /* Set target value */
  vector_set_value( tgt, vec.value, tgt->width, 0, 0 );

}

/*!
 \param tgt  Target vector for operation results to be stored.
 \param src  Source vector to perform operation on.

 Performs a bitwise inversion on the specified vector.
*/
void vector_unary_inv( vector* tgt, vector* src ) {

  nibble bit;      /* Selected bit from source vector */
  vector vec;      /* Temporary vector value          */
  nibble vec_val;  /* Temporary value                 */
  int    i;        /* Loop iterator                   */

  vector_init( &vec, &vec_val, 1 );

  for( i=0; i<src->width; i++ ) {

    bit = VECTOR_VAL( src->value[i] );

    switch( bit ) {
      case 0  :  VECTOR_SET_VAL( vec.value[0], 1 );  break;
      case 1  :  VECTOR_SET_VAL( vec.value[0], 0 );  break;
      default :  VECTOR_SET_VAL( vec.value[0], 2 );  break;
    }
    vector_set_value( tgt, vec.value, 1, 0, i );

  }

}

/*!
 \param tgt    Target vector for operation result storage.
 \param src    Source vector to be operated on.
 \param optab  Operation table.

 Performs unary operation on specified vector value from specifed
 operation table.
*/
void vector_unary_op( vector* tgt, vector* src, nibble* optab ) {

  nibble uval;     /* Unary operation value        */
  nibble bit;      /* Current bit under evaluation */
  vector vec;      /* Temporary vector value       */
  nibble vec_val;  /* Temporary value              */
  int    i;        /* Loop iterator                */

  if( (src->width == 1) && (optab[16] == 1) ) {

    /* Perform inverse operation if our source width is 1 and we are a NOT operation. */
    vector_unary_inv( tgt, src );

  } else {

    vector_init( &vec, &vec_val, 1 );

    uval = VECTOR_VAL( src->value[0] );

    for( i=1; i<src->width; i++ ) {
      bit  = VECTOR_VAL( src->value[i] );
      uval = optab[ ((uval << 2) | bit) ]; 
    }

    VECTOR_SET_VAL( vec.value[0], uval );
    vector_set_value( tgt, vec.value, 1, 0, 0 );

  }

}

/*!
 \param tgt  Target vector for operation result storage.
 \param src  Source vector to be operated on.

 Performs unary logical NOT operation on specified vector value.
*/
void vector_unary_not( vector* tgt, vector* src ) {

  vector_unary_op( tgt, src, nor_optab );

}

/*!
 \param vec  Pointer to vector to deallocate memory from.

 Deallocates all heap memory that was initially allocated with the malloc
 routine.
*/
void vector_dealloc( vector* vec ) {

  if( vec != NULL ) {

    /* Deallocate all vector values */
    if( vec->value != NULL ) {
      free_safe( vec->value );
      vec->value = NULL;
    }

    /* Deallocate vector itself */
    free_safe( vec );

  } else {

    /* Internal error, we should never be trying to deallocate a NULL vector */
    assert( vec != NULL );

  }

}

/*
 $Log$
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

