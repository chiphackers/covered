/*!
 \file     vector.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 
 \par
 A vector comprised of three components
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef MALLOC_DEBUG
#include <mpatrol.h>
#endif

#include "defines.h"
#include "vector.h"
#include "util.h"


nibble xor_optab[16]  = { XOR_OP_TABLE  };  /*!< XOR operation table  */
nibble and_optab[16]  = { AND_OP_TABLE  };  /*!< AND operation table  */
nibble or_optab[16]   = { OR_OP_TABLE   };  /*!< OR operation table   */
nibble nand_optab[16] = { NAND_OP_TABLE };  /*!< NAND operation table */
nibble nor_optab[16]  = { NOR_OP_TABLE  };  /*!< NOR operation table  */
nibble nxor_optab[16] = { NXOR_OP_TABLE };  /*!< NXOR operation table */
nibble add_optab[16]  = { ADD_OP_TABLE  };  /*!< ADD operation table  */

extern char user_msg[USER_MSG_LENGTH];


/*!
 \param vec    Pointer to vector to initialize.
 \param value  Pointer to nibble array for vector.
 \param width  Bit width of specified vector.
 \param lsb    Least-significant bit of vector.
 
 Initializes the specified vector with the contents of width, lsb
 and value (if value != NULL).  If value != NULL, initializes all contents 
 of value array to 0x2 (X-value).
*/
void vector_init( vector* vec, nibble* value, int width, int lsb ) {

  int i;        /* Loop iterator */

  assert( lsb >= 0 );

  vec->width = width;
  vec->lsb   = lsb;
  vec->value = value;

  if( value != NULL ) {

    assert( width > 0 );

    for( i=0; i<VECTOR_SIZE( width ); i++ ) {
      vec->value[i] = 0x0;
    }

  }

}

/*!
 \param width  Bit width of this vector.
 \param lsb    Least significant bit for this vector.
 \param data   If FALSE only initializes width and lsb but does not allocate a nibble array.

 \return Pointer to newly created vector.

 Creates new vector from heap memory and initializes all vector contents.
*/
vector* vector_create( int width, int lsb, bool data ) {

  vector* new_vec;       /* Pointer to newly created vector               */
  nibble* value = NULL;  /* Temporarily stores newly created vector value */

  assert( width > 0 );

  new_vec = (vector*)malloc_safe( sizeof( vector ) );

  if( data == TRUE ) {
    value = (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_SIZE( width ) );
  }

  vector_init( new_vec, value, width, lsb );

  return( new_vec );

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

  mask = write_data ? 0x3fffff : 0x3fff00;

  /* Output vector information to specified file */
  fprintf( file, "%d %d",
    vec->width,
    vec->lsb
  );

  assert( vec->width < MAX_BIT_WIDTH );

  if( vec->value == NULL ) {

    /* If the vector value was NULL, output default value */
    for( i=0; i<VECTOR_SIZE( vec->width ); i++ ) {
      if( i == 0 ) {
        fprintf( file, " aa" );
      } else {
        fprintf( file, ", aa" );
      }
    }

  } else {

    for( i=0; i<VECTOR_SIZE( vec->width ); i++ ) {
      if( i == 0 ) {
        fprintf( file, " %x", ((vec->value[i] & mask) | (write_data ? 0 : 0xaa)) );
      } else {
        fprintf( file, ",%x", ((vec->value[i] & mask) | (write_data ? 0 : 0xaa)) );
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

  bool  retval = TRUE;                    /* Return value for this function   */
  int   width;                            /* Vector bit width                 */
  int   lsb;                              /* Vector LSB value                 */
  int   i;                                /* Loop iterator                    */
  int   chars_read;                       /* Number of characters read        */

  /* Read in vector information */
  if( sscanf( *line, "%d %d%n", &width, &lsb, &chars_read ) == 2 ) {

    *line = *line + chars_read;

    /* Create new vector */
    *vec = vector_create( width, lsb, TRUE );

    sscanf( *line, "%x%n", &((*vec)->value[0]), &chars_read );
    *line = *line + chars_read;
 
    i = 1;
    while( sscanf( *line, ",%x%n", &((*vec)->value[i]), &chars_read ) == 1 ) {
      *line = *line + chars_read;
      i++;
    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Base vector to merge data into.
 \param line  Pointer to line to parse for vector information.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Parses current file line for vector information and performs vector merge of 
 base vector and read vector information.  If the vectors are found to be different
 (width or lsb are not equal), an error message is sent to the user and the
 program is halted.  If the vectors are found to be equivalents, the merge is
 performed on the vector nibbles.
*/
bool vector_db_merge( vector* base, char** line ) {

  bool   retval = TRUE;   /* Return value of this function */
  int    width;           /* Width of read vector          */
  int    lsb;             /* LSB of read vector            */
  nibble data;            /* Read in data nibble           */
  int    chars_read;      /* Number of characters read     */
  int    i;               /* Loop iterator                 */

  assert( base != NULL );

  if( sscanf( *line, "%d %d%n", &width, &lsb, &chars_read ) == 2 ) {

    *line = *line + chars_read;

    if( (base->width != width) || (base->lsb != lsb) ) {

      print_output( "Attempting to merge databases derived from different designs.  Unable to merge", FATAL );
      exit( 1 );

    } else {

      sscanf( *line, "%x%n", &data, &chars_read );
      *line = *line + chars_read;
      base->value[0] = (base->value[0] & VECTOR_MERGE_MASK) | (data & VECTOR_MERGE_MASK);

      i = 1;
      while( sscanf( *line, ",%x%n", &data, &chars_read ) == 1 ) {
        *line = *line + chars_read;
        base->value[i] = (base->value[i] & VECTOR_MERGE_MASK) | (data & VECTOR_MERGE_MASK);
        i++;
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
 \param lsb    Least significant bit of vector to display
 \param ofile  Stream to output information to.
 
 Displays the toggle01 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle01( nibble* nib, int width, int lsb, FILE* ofile ) {

  int i;    /* Loop iterator */

  fprintf( ofile, "%d'b", width );

  for( i=((width - 1)+lsb); i>=lsb; i-- ) {
    fprintf( ofile, "%d", ((nib[(i/4)] >> ((i%4)+8)) & 0x1) );
  }

}

/*!
 \param nib    Nibble to display toggle information
 \param width  Number of bits of nibble to display
 \param lsb    Least significant bit of vector to display
 \param ofile  Stream to output information to.
 
 Displays the toggle10 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle10( nibble* nib, int width, int lsb, FILE* ofile ) {

  int i;    /* Loop iterator */

  fprintf( ofile, "%d'b", width );

  for( i=((width - 1)+lsb); i>=lsb; i-- ) {
    fprintf( ofile, "%d", ((nib[(i/4)] >> ((i%4)+12)) & 0x1) );
  }

}

/*!
 \param nib    Nibble to display.
 \param width  Number of bits in nibble to display.
 \param lsb    Least significant bit of specified vector.

 Outputs the specified nibble array to standard output as described by the
 width and lsb parameters.
*/
void vector_display_nibble( nibble* nib, int width, int lsb ) {

  int i;       /* Loop iterator */

  printf( "\n" );
  printf( "      raw value:" );
  
  for( i=((width%4)==0)?((width/4)-1):(width/4); i>=0; i-- ) {
    printf( " %08x", nib[i] );
  }

  /* Display nibble value */
  printf( ", value: %d'b", width );

  for( i=((width - 1)+lsb); i>=lsb; i-- ) {
    switch( (nib[(i/4)] >> ((i%4)*2)) & 0x3 ) {
      case 0 :  printf( "0" );  break;
      case 1 :  printf( "1" );  break;
      case 2 :  printf( "x" );  break;
      case 3 :  printf( "z" );  break;
      default:  break;
    }
  }

  /* Display nibble toggle01 history */
  printf( ", 0->1: " );
  vector_display_toggle01( nib, width, lsb, stdout );

  /* Display nibble toggle10 history */
  printf( ", 1->0: " );
  vector_display_toggle10( nib, width, lsb, stdout );

  /* Display bit set information */
  printf( ", set: %d'b", width );

  for( i=((width - 1)+lsb); i>=lsb; i-- ) {
    printf( "%d", ((nib[(i/4)] >> ((i%4)+16)) & 0x1) );
  }

  /* Display bit FALSE information */
  printf( ", FALSE: %d'b", width );

  for( i=((width - 1)+lsb); i>=lsb; i-- ) {
    printf( "%d", ((nib[(i/4)] >> ((i%4)+24)) & 0x1) );
  }

  /* Display bit TRUE information */
  printf( ", TRUE: %d'b", width );

  for( i=((width - 1)+lsb); i>=lsb; i-- ) {
    printf( "%d", ((nib[(i/4)] >> ((i%4)+28)) & 0x1) );
  }

}

/*!
 \param vec  Pointer to vector to output to standard output.

 Outputs contents of vector to standard output (for debugging purposes only).
*/
void vector_display( vector* vec ) {

  assert( vec != NULL );

  printf( "Vector => width: %d, lsb: %d, ", vec->width, vec->lsb );

  if( (vec->lsb >= 0) && (vec->width > 0) ) {
    vector_display_nibble( vec->value, vec->width, vec->lsb );
  }

  printf( "\n" );

}


/*!
 \param value  Vector value to retrieve bit value from.
 \param pos    Bit position to extract
 \return 2-bit (4-state) value of specified bit from specified vector.

 Assumes that pos is within the range of value.  Retrieves bit value
 specified from pos index from the specified value, byte-aligns the value
 and returns this 2-bit value to the calling function.  Used to extract
 bit values from a vector.
*/
nibble vector_bit_val( nibble* value, int pos ) {

  nibble bit_val = 0;   /* Extracted bit value                                */
  int    bit_shift;     /* Number of bits to shift in nibble to get bit value */
  int    nibble_to_get; /* Nibble index where value is located                */

  nibble_to_get = (pos / 4);
  bit_shift     = (pos % 4) * 2;

  bit_val = (value[nibble_to_get] >> bit_shift) & 0x3;

  return( bit_val );

}

/*!
 \param nib    Nibble vector to set bit to.
 \param value  2-bit (4-state) value to set bit to.
 \param pos    Bit position to set.

 Sets bit in value portion of nibble (bits 7-0) to specified 4-state value.
*/
void vector_set_bit( nibble* nib, nibble value, int pos ) {

  nib[(pos/4)] = (nib[(pos/4)] & ~(0x3 << ((pos%4)*2))) | ((value & 0x3) << ((pos%4)*2));

} 

/*!
 \param value  Nibble array containing toggle vector to set.
 \param pos    Bit position in toggle vector to set.

 Sets the specified bit in the toggle vector according to the pos index.
 This function assumes that the specified bit position is within the
 range of toggle.
*/
void vector_set_toggle01( nibble* value, int pos ) {

  int bit_shift;      /* Bit position of bit in character       */
  int nibble_to_set;  /* Nibble index where toggled bit resides */

  nibble_to_set = (pos / 4);
  bit_shift     = (pos % 4) + 8;

  value[nibble_to_set] = value[nibble_to_set] | (0x1 << bit_shift);

}

/*!
 \param value  Nibble array containing toggle vector to set.
 \param pos    Bit position in toggle vector to set.

 Sets the specified bit in the toggle vector according to the pos index.
 This function assumes that the specified bit position is within the
 range of toggle.
*/
void vector_set_toggle10( nibble* value, int pos ) {

  int bit_shift;      /* Bit position of bit in character       */
  int nibble_to_set;  /* Nibble index where toggled bit resides */

  nibble_to_set = (pos / 4);
  bit_shift     = (pos % 4) + 12;

  value[nibble_to_set] = value[nibble_to_set] | (0x1 << bit_shift);

}

/*!
 \param value  Nibble array containing FALSE vector to set.
 \param pos    Bit position in FALSE vector to set.

 Sets the specified bit in the FALSE vector according to the pos index.
 This function assumes that the specified bit position is within the
 range of FALSE.
*/
void vector_set_false( nibble* value, int pos ) {

  int bit_shift;      /* Bit position of bit in character     */
  int nibble_to_set;  /* Nibble index where FALSE bit resides */

  nibble_to_set = (pos / 4);
  bit_shift     = (pos % 4) + 24;

  value[nibble_to_set] = value[nibble_to_set] | (0x1 << bit_shift);

}

/*!
 \param value  Nibble array containing TRUE vector to set.
 \param pos    Bit position in TRUE vector to set.

 Sets the specified bit in the TRUE vector according to the pos index.
 This function assumes that the specified bit position is within the
 range of TRUE.
*/
void vector_set_true( nibble* value, int pos ) {

  int bit_shift;      /* Bit position of bit in character     */
  int nibble_to_set;  /* Nibble index where TRUE bit resides */

  nibble_to_set = (pos / 4);
  bit_shift     = (pos % 4) + 28;

  value[nibble_to_set] = value[nibble_to_set] | (0x1 << bit_shift);

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

  int    i;     /* Loop iterator                  */
  int    j;     /* Loop iterator                  */
  nibble curr;  /* Current nibble being evaluated */

  for( i=0; i<VECTOR_SIZE( vec->width ); i++ ) {
    curr = vec->value[i];
    for( j=0; j<4; j++ ) {
      *tog01_cnt = *tog01_cnt + ((curr >> (j +  8)) & 0x1);
      *tog10_cnt = *tog10_cnt + ((curr >> (j + 12)) & 0x1);
    }
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

  int    i;     /* Loop iterator                  */
  int    j;     /* Loop iterator                  */
  nibble curr;  /* Current nibble being evaluated */

  for( i=0; i<VECTOR_SIZE( vec->width ); i++ ) {
    curr = vec->value[i];
    for( j=0; j<4; j++ ) {
      *false_cnt = *false_cnt + ((curr >> (j + 24)) & 0x1);
      *true_cnt  = *true_cnt  + ((curr >> (j + 28)) & 0x1);
    }
  }

}

/*!
 \param vec       Pointer to vector to set value to.
 \param value     New value to set vector value to.
 \param width     Width of new value.
 \param from_idx  Starting bit index of value to start copying.
 \param to_idx    Starting bit index of vec value to copy to.
 \return Returns TRUE if assignment was successful; otherwise, returns FALSE.

 Allows the calling function to set any bit vector within the vector
 range.  If the vector value has never been set, sets
 the value to the new value and returns.  If the vector value has previously
 been set, checks to see if new vector bits have toggled, sets appropriate
 toggle values, sets the new value to this value and returns.
*/
void vector_set_value( vector* vec, nibble* value, int width, int from_idx, int to_idx ) {

  nibble from_val;      /* Current bit value of value being assigned  */
  nibble to_val;        /* Current bit value of previous value        */
  int    nibble_to_set; /* Index to current nibble in value           */
  int    bit_shift;     /* Number of bits to shift in current nibble  */
  int    i;             /* Loop iterator                              */
  nibble value_changed; /* Indicates if current bit has changed value */

  assert( vec != NULL );

  /* Verify that index is within range */
  /* printf( "to_idx: %d, vec->width: %d, vec->lsb: %d\n", to_idx, vec->width, vec->lsb ); */
  assert( to_idx < (vec->width + vec->lsb) );
  assert( to_idx >= 0 );

  /* Adjust width to smaller of two values */
  width = (width > (vec->width - to_idx)) ? (vec->width - to_idx) : width;

  for( i=0; i<width; i++ ) {

    from_val = vector_bit_val( value, (i + from_idx) );

    if( ((vec->value[i/4] >> ((i%4)+16)) & 0x1) == 0x1 ) {

      /* Assign toggle values if necessary */
      to_val = vector_bit_val( vec->value, (i + to_idx) );

      if( (to_val == 0) && (from_val == 1) ) {
        vector_set_toggle01( vec->value, (i + to_idx) );
      } else if( (to_val == 1) && (from_val == 0) ) {
        vector_set_toggle10( vec->value, (i + to_idx) );
      }

    }

    /* Assign TRUE/FALSE values if necessary */
    if( from_val == 0 ) {
      vector_set_false( vec->value, (i + to_idx) );
    } else if( from_val == 1 ) {
      vector_set_true( vec->value, (i + to_idx) );
    }

  }

  /* Perform value assignment */
  for( i=0; i<width; i++ ) {

    nibble_to_set = ((i + to_idx) / 4);
    bit_shift     = ((i + to_idx) % 4) * 2;

    /* If value has changed from previous, set previous assignment bit */
    if( vector_bit_val( value, (i + from_idx) ) != vector_bit_val( vec->value, (i + to_idx )) ) {
      value_changed = (0x1 << (((i + to_idx) % 4) + 16));
    } else {
      value_changed = 0;
    }

    vec->value[nibble_to_set] = value_changed |
                                (vec->value[nibble_to_set] & ~(0x3 << bit_shift)) |
                                (vector_bit_val( value, (i + from_idx) ) << bit_shift);

  }

}

/*!
 \param vec   Pointer to vector to set type.
 \param type  2-bit value containing output type for this vector (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).

 Uses the first nibble in the specified vector and stores specified 2-bit type value to bits 21-20
 of this nibble.
*/
void vector_set_type( vector* vec, int type ) {

  assert( vec->value != NULL );

  vec->value[0] = vec->value[0] | ((type & 0x3) << 20);

}

/*!
 \param vec  Pointer to vector to retrieve type from.

 \return Returns the 2-bit output type value from the specified vector.

 Returns the 2-bit output type value from the specified vector's first
 nibble bits 21-20.
*/
int vector_get_type( vector* vec ) {

  assert( vec->value != NULL );

  return( (vec->value[0] >> 20) & 0x3 );

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

  for( i=(width - 1); i>=vec->lsb; i-- ) {
    switch( (vec->value[i/4] >> ((i%4)*2)) & 0x3 ) {
      case 0 :  retval = (retval << 1) | 0;  break;
      case 1 :  retval = (retval << 1) | 1;  break;
      default:
        print_output( "Vector converting to integer contains X or Z values", FATAL );
        exit( 1 );
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
    vector_set_bit( vec->value, (value & 0x1), i );
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
          vector_set_bit( vec->value, 0x2, (i + pos) );
        }
      } else if( (*ptr == 'z') || (*ptr == 'Z') || (*ptr == '?') ) {
        for( i=0; i<bits_per_char; i++ ) {
          vector_set_bit( vec->value, 0x3, (i + pos) );
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
  	  vector_set_bit( vec->value, ((val >> i) & 0x1), (i + pos) );
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
    switch( vector_bit_val( vec->value, i ) ) {
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
 sized.
*/
vector* vector_from_string( char* str ) {

  vector* vec;                   /* Temporary holder for newly created vector                                */
  int     bits_per_char;         /* Number of bits represented by a single character in the value string str */
  int     size;                  /* Specifies bit width of vector to create                                  */
  char    value[MAX_BIT_WIDTH];  /* String to store string value in                                          */
  char    stype[2];              /* Temporary holder for type of string being parsed                         */
  int     type;                  /* Type of string being parsed                                              */

  if( sscanf( str, "%d'%[sSdD]%[0-9]", &size, stype, value ) == 3 ) {
    bits_per_char = 10;
    type          = DECIMAL;
  } else if( sscanf( str, "%d'%[sSbB]%[01xXzZ_\?]", &size, stype, value ) == 3 ) {
    bits_per_char = 1;
    type          = BINARY;
  } else if( sscanf( str, "%d'%[sSoO]%[0-7xXzZ_\?]", &size, stype, value ) == 3 ) {
    bits_per_char = 3;
    type          = OCTAL;
  } else if( sscanf( str, "%d'%[sShH]%[0-9a-fA-FxXzZ_\?]", &size, stype, value ) == 3 ) {
    bits_per_char = 4;
    type          = HEXIDECIMAL;
  } else if( sscanf( str, "'%[sSdD]%[0-9]", stype, value ) == 2 ) {
    bits_per_char = 10;
    type          = DECIMAL;
    size          = 32;
  } else if( sscanf( str, "'%[sSbB]%[01xXzZ_\?]", stype, value ) == 2 ) {
    bits_per_char = 1;
    type          = BINARY;
    size          = 32;
  } else if( sscanf( str, "'%[sSoO]%[0-7xXzZ_\?]", stype, value ) == 2 ) {
    bits_per_char = 3;
    type          = OCTAL;
    size          = 32;
  } else if( sscanf( str, "'%[sShH]%[0-9a-fA-FxXzZ_\?]", stype, value ) == 2 ) {
    bits_per_char = 4;
    type          = HEXIDECIMAL;
    size          = 32;
  } else if( sscanf( str, "%[0-9_]", value ) == 1 ) {
    bits_per_char = 10;
    type          = DECIMAL;
    size          = 32;
  } else {
    /* If the specified string is none of the above, return NULL */
    return( NULL );
  }

  /* Create vector */
  vec = vector_create( size, 0, TRUE );

  vector_set_type( vec, type );

  if( type == DECIMAL ) {
    vector_from_int( vec, atol( value ) );
  } else {
    vector_set_static( vec, value, bits_per_char ); 
  }

  return( vec );

}

/*!
 \param vec    Pointer to vector to set value to.
 \param value  String version of VCD value.

 Iterates through specified value string, setting the specified vector value to
 this value.  Performs a VCD-specific bit-fill if the value size is not the size
 of the vector.  The specified value string is assumed to be in binary format.
*/
void vector_vcd_assign( vector* vec, char* value ) {

  char*  ptr;   /* Pointer to current character under evaluation */
  int    i;     /* Loop iterator                                 */
  nibble vval;  /* Temporary vector value holder                 */

  assert( vec != NULL );
  assert( value != NULL );

  /* Set pointer to LSB */
  ptr = (value + strlen( value )) - 1;
  i   = 0;
    
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
  for( ; i<vec->width; i++ ) {
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

  vector_init( &vec, &vecval, 1, 0 );

  for( i=0; i<tgt->width; i++ ) {

    if( src1->width > i ) {
      bit1 = vector_bit_val( src1->value, i );
    } else {
      bit1 = 0;
    }

    if( src2->width > i ) {
      bit2 = vector_bit_val( src2->value, i );
    } else {
      bit2 = 0;
    }

    vector_set_bit( vec.value, optab[ ((bit1 << 2) | bit2) ], 0 );
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

  /* Determine at which bit position to begin comparing */
  if( left->width > right->width ) {
    pos = left->width - 1;
  } else {
    pos = right->width - 1;
  }

  while( (pos >= 0) && !done ) {

    if( pos < left->width ) {
      lbit = vector_bit_val( left->value, pos );
    } else {
      lbit = 0;
    }

    if( pos < right->width ) {
      rbit = vector_bit_val( right->value, pos );
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
  nibble zero = 0;     /* Zero value for zero-fill     */
  int    i;            /* Loop iterator                */

  shift_val = vector_to_int( right );

  if( shift_val >= left->width ) {
   
    shift_val = left->width;

  } else {

    vector_set_value( tgt, left->value, (left->width - shift_val), left->lsb, shift_val );

  }

  /* Zero-fill LSBs */
  for( i=0; i<shift_val; i++ ) {
    vector_set_value( tgt, &zero, 1, 0, i );
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
  nibble zero = 0;     /* Zero value for zero-fill     */
  int    i;            /* Loop iterator                */

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
      lbit = vector_bit_val( left->value, i );
    } else {
      lbit = 0;
    }

    if( i < right->width ) {
      rbit = vector_bit_val( right->value, i );
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
  vec1 = vector_create( tgt->width, 0, TRUE );
  vec2 = vector_create( tgt->width, 0, TRUE );
  vec3 = vector_create( tgt->width, 0, TRUE );

  /* Create vector with a value of 1 */
  vector_set_bit( vec1->value, 1, 0 );

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

  vector lcomp;                         /* Compare vector left  */
  nibble lcomp_val;                     /* Compare value left   */
  vector rcomp;                         /* Compare vector right */
  nibble rcomp_val;                     /* Compare value right  */
  vector vec;                           /* Intermediate vector  */
  nibble vec_val[ VECTOR_SIZE( 32 ) ];  /* Intermediate value   */
  int     i;                            /* Loop iterator        */

  /* Initialize temporary vectors */
  vector_init( &lcomp, &lcomp_val, 1, 0 );
  vector_init( &rcomp, &rcomp_val, 1, 0 );
  vector_init( &vec,   vec_val,   32, 0 );

  vector_unary_op( &lcomp, left,  xor_optab );
  vector_unary_op( &rcomp, right, xor_optab );

  /* Perform 4-state multiplication */
  if( ((lcomp.value[0] & 0x3) == 2) && ((rcomp.value[0] & 0x3) == 2) ) {

    for( i=0; i<vec.width; i++ ) {
      vector_set_bit( vec.value, 2, i );
    }

  } else if( ((lcomp.value[0] & 0x3) != 2) && ((rcomp.value[0] & 0x3) == 2) ) {

    if( vector_to_int( left ) == 0 ) {
      vector_from_int( &vec, 0 );
    } else if( vector_to_int( left ) == 1 ) {
      vector_set_value( &vec, right->value, right->width, right->lsb, 0 );
    } else {
      for( i=0; i<vec.width; i++ ) {
        vector_set_bit( vec.value, 2, i );
      }
    }

  } else if( ((lcomp.value[0] & 0x3) == 2) && ((rcomp.value[0] & 0x3) != 2) ) {

    if( vector_to_int( right ) == 0 ) {
      vector_from_int( &vec, 0 );
    } else if( vector_to_int( right ) == 1 ) {
      vector_set_value( &vec, left->value, left->width, left->lsb, 0 );
    } else {
      for( i=0; i<vec.width; i++ ) {
        vector_set_bit( vec.value, 2, i );
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

  vector_init( &vec, &vec_val, 1, 0 );

  for( i=0; i<src->width; i++ ) {

    bit = vector_bit_val( src->value, (i + src->lsb) );

    switch( bit ) {
      case 0  :  vector_set_bit( vec.value, 1, 0 );  break;
      case 1  :  vector_set_bit( vec.value, 0, 0 );  break;
      default :  vector_set_bit( vec.value, 2, 0 );  break;
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

  vector_init( &vec, &vec_val, 1, 0 );

  uval = vector_bit_val( src->value, src->lsb );

  for( i=1; i<src->width; i++ ) {
    bit  = vector_bit_val( src->value, (i + src->lsb) );
    uval = optab[ ((uval << 2) | bit) ]; 
  }

  vector_set_bit( vec.value, uval, 0 );
  vector_set_value( tgt, vec.value, 1, 0, 0 );

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

