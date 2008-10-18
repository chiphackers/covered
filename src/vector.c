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
*/

#define _ISOC99_SOURCE 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#ifdef MALLOC_DEBUG
#include <mpatrol.h>
#endif

#include "defines.h"
#include "vector.h"
#include "util.h"


/*!
 Returns the number of unsigned long elements are required to store a vector with a bit width of width.
*/
#define UL_SIZE(width)      (UL_DIV((width) - 1) + 1)

/*! Lower mask */
#define UL_LMASK(lsb)       (UL_SET << UL_MOD(lsb))

/*! Upper mask */
#define UL_HMASK(msb)       (UL_SET >> ((UL_BITS - 1) - UL_MOD(msb)))


/*! Contains the structure sizes for the various vector types (vector "type" supplemental field is the index to this array */
static const unsigned int vector_type_sizes[4] = {VTYPE_INDEX_VAL_NUM, VTYPE_INDEX_SIG_NUM, VTYPE_INDEX_EXP_NUM, VTYPE_INDEX_MEM_NUM};

extern char   user_msg[USER_MSG_LENGTH];
extern isuppl info_suppl;


/*!
 Initializes the specified vector with the contents of width
 and value (if value != NULL).  If value != NULL, initializes all contents 
 of value array to 0x2 (X-value).
*/
void vector_init_ulong(
  vector* vec,         /*!< Pointer to vector to initialize */
  ulong** value,       /*!< Pointer to vec_data array for vector */
  ulong   data_l,      /*!< Initial value to set each lower data value to */
  ulong   data_h,      /*!< Initial value to set each upper data value to */
  bool    owns_value,  /*!< Set to TRUE if this vector is responsible for deallocating the given value array */
  int     width,       /*!< Bit width of specified vector */
  int     type         /*!< Type of vector to initialize this to */
) { PROFILE(VECTOR_INIT);

  vec->width                = width;
  vec->suppl.all            = 0;
  vec->suppl.part.type      = type;
  vec->suppl.part.data_type = VDATA_UL;
  vec->suppl.part.owns_data = owns_value;
  vec->value.ul             = value;

  if( value != NULL ) {

    int    i, j;
    int    size  = UL_SIZE(width);
    int    num   = vector_type_sizes[type];
    ulong  lmask = UL_HMASK(width - 1);

    assert( width > 0 );

    for( i=0; i<(size - 1); i++ ) {
      vec->value.ul[i][VTYPE_INDEX_VAL_VALL] = data_l;
      vec->value.ul[i][VTYPE_INDEX_VAL_VALH] = data_h;
      for( j=2; j<num; j++ ) {
        vec->value.ul[i][j] = 0x0;
      }
    }

    vec->value.ul[i][VTYPE_INDEX_VAL_VALL] = data_l & lmask;
    vec->value.ul[i][VTYPE_INDEX_VAL_VALH] = data_h & lmask;
    for( j=2; j<num; j++ ) {
      vec->value.ul[i][j] = 0x0;
    }

  } else {

    assert( !owns_value );

  }

  PROFILE_END;

}

/*!
 Initializes the specified vector with the contents of width and value (if value != NULL).
 If value != NULL, initializes all contents of value array to NaN.
*/
void vector_init_r64(
  vector*      vec,         /*!< Pointer to vector to initialize */
  rv64*        value,       /*!< Pointer to real value structure for vector */
  double       data,        /*!< Initial value to set the data to */
  char*        str,         /*!< String representation of the value */
  bool         owns_value,  /*!< Set to TRUE if this vector is responsible for deallocating the given value array */
  int          type         /*!< Type of vector to initialize this to */
) { PROFILE(VECTOR_INT_R64);

  vec->width                = 64;
  vec->suppl.all            = 0;
  vec->suppl.part.type      = type;
  vec->suppl.part.data_type = VDATA_R64;
  vec->suppl.part.owns_data = owns_value;
  vec->value.r64            = value;

  if( value != NULL ) {

    vec->value.r64->val = data;
    vec->value.r64->str = (str != NULL) ? strdup_safe( str ) : NULL;

  } else {

    assert( !owns_value );

  }

  PROFILE_END;

}

/*!
 Initializes the specified vector with the contents of width and value (if value != NULL).
 If value != NULL, initializes all contents of value array to NaN.
*/
void vector_init_r32(
  vector*      vec,         /*!< Pointer to vector to initialize */
  rv32*        value,       /*!< Pointer to 32-bit real value structure for vector */
  float        data,        /*!< Initial value to set the data to */
  char*        str,         /*!< String representation of the value */
  bool         owns_value,  /*!< Set to TRUE if this vector is responsible for deallocating the given value array */
  int          type         /*!< Type of vector to initialize this to */
) { PROFILE(VECTOR_INT_R32);

  vec->width                = 32;
  vec->suppl.all            = 0;
  vec->suppl.part.type      = type;
  vec->suppl.part.data_type = VDATA_R32;
  vec->suppl.part.owns_data = owns_value;
  vec->value.r32            = value;

  if( value != NULL ) {

    vec->value.r32->val = data;
    vec->value.r32->str = (str != NULL) ? strdup_safe( str ) : NULL;

  } else {

    assert( !owns_value );

  }

  PROFILE_END;

}

/*!
 \return Pointer to newly created vector.

 Creates new vector from heap memory and initializes all vector contents.
*/
vector* vector_create(
  int  width,      /*!< Bit width of this vector */
  int  type,       /*!< Type of vector to create (see \ref vector_types for valid values) */
  int  data_type,  /*!< Data type used to store vector value information */
  bool data        /*!< If FALSE only initializes width but does not allocate a value array */
) { PROFILE(VECTOR_CREATE);

  vector* new_vec;  /* Pointer to newly created vector */

  assert( width > 0 );

  new_vec = (vector*)malloc_safe( sizeof( vector ) );

  switch( data_type ) {
    case VDATA_UL :
      {
        ulong** value = NULL;
        if( data == TRUE ) {
          int          num  = vector_type_sizes[type];
          unsigned int size = UL_SIZE(width);
          unsigned int i;
          value = (ulong**)malloc_safe( sizeof( ulong* ) * size );
          for( i=0; i<size; i++ ) {
            value[i] = (ulong*)malloc_safe( sizeof( ulong ) * num );
          }
        }
        vector_init_ulong( new_vec, value, 0x0, 0x0, (value != NULL), width, type );
      }
      break;
    case VDATA_R64 :
      {
        rv64* value = NULL;
        if( data == TRUE ) {
          value = (rv64*)malloc_safe( sizeof( rv64 ) );
        }
        vector_init_r64( new_vec, value, 0.0, NULL, (value != NULL), type );
      }
      break;
    case VDATA_R32 :
      {
        rv32* value = NULL;
        if( data == TRUE ) {
          value = (rv32*)malloc_safe( sizeof( rv32 ) );
        }
        vector_init_r32( new_vec, value, 0.0, NULL, (value != NULL), type );
      }
      break;
    default :  assert( 0 );
  }

  PROFILE_END;

  return( new_vec );

}

/*!
 Copies the contents of the from_vec to the to_vec.
*/
void vector_copy(
  const vector* from_vec,  /*!< Vector to copy */
  vector*       to_vec     /*!< Newly created vector copy */
) { PROFILE(VECTOR_COPY);

  unsigned int i, j;  /* Loop iterators */

  assert( from_vec != NULL );
  assert( to_vec != NULL );
  assert( from_vec->width == to_vec->width );
  assert( from_vec->suppl.part.data_type == to_vec->suppl.part.data_type );

  switch( to_vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int size      = UL_SIZE( from_vec->width );
        unsigned int type_size = (from_vec->suppl.part.type != to_vec->suppl.part.type) ? 2 : vector_type_sizes[to_vec->suppl.part.type];
        for( i=0; i<size; i++ ) {
          for( j=0; j<type_size; j++ ) {
            to_vec->value.ul[i][j] = from_vec->value.ul[i][j];
          }
        }
      }
      break;
    case VDATA_R64 :
      {
        to_vec->value.r64->val = from_vec->value.r64->val;
        to_vec->value.r64->str = (from_vec->value.r64->str != NULL) ? strdup_safe( from_vec->value.r64->str ) : NULL;
      }
      break;
    case VDATA_R32 :
      {
        to_vec->value.r32->val = from_vec->value.r32->val;
        to_vec->value.r32->str = (from_vec->value.r32->str != NULL) ? strdup_safe( from_vec->value.r32->str ) : NULL;
      }
      break;
    default:  assert( 0 );  break;
  }

  PROFILE_END;

}

/*!
 Copies the entire contents of a bit range from from_vec to to_vec,
 aligning the stored value starting at bit 0.
*/
void vector_copy_range(
  vector*       to_vec,    /*!< Vector to copy to */
  const vector* from_vec,  /*!< Vector to copy from */
  int           lsb        /*!< LSB of bit range to copy */
) { PROFILE(VECTOR_COPY_RANGE);

  assert( from_vec != NULL );
  assert( to_vec != NULL );
  assert( from_vec->suppl.part.type == to_vec->suppl.part.type );
  assert( from_vec->suppl.part.data_type == to_vec->suppl.part.data_type );

  switch( to_vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i, j;
        for( i=0; i<to_vec->width; i++ ) {
          unsigned int my_index     = UL_DIV(i);
          unsigned int their_index  = UL_DIV(i + lsb);
          unsigned int their_offset = UL_MOD(i + lsb);
          for( j=0; j<vector_type_sizes[to_vec->suppl.part.type]; j++ ) {
            if( UL_MOD(i) == 0 ) {
              to_vec->value.ul[my_index][j] = 0;
            }
            to_vec->value.ul[my_index][j] |= (((from_vec->value.ul[their_index][j] >> their_offset) & 0x1) << i);
          }
        }
      }
      break;
    case VDATA_R32 :
    case VDATA_R64 :
      assert( 0 );
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

}

/*!
 Copies the contents of the from_vec to the to_vec, allocating new memory.
*/
void vector_clone(
            const vector* from_vec,  /*!< Vector to copy */
  /*@out@*/ vector**      to_vec     /*!< Newly created vector copy */
) { PROFILE(VECTOR_CLONE);

  if( from_vec == NULL ) {

    /* If from_vec is NULL, just assign to_vec to NULL */
    *to_vec = NULL;

  } else {

    /* Create vector */
    *to_vec = vector_create( from_vec->width, from_vec->suppl.part.type, from_vec->suppl.part.data_type, TRUE );

    vector_copy( from_vec, *to_vec );

  }

  PROFILE_END;

}

/*!
 Writes the specified vector to the specified coverage database file.
*/
void vector_db_write(
  vector* vec,         /*!< Pointer to vector to display to database file */
  FILE*   file,        /*!< Pointer to coverage database file to display to */
  bool    write_data,  /*!< If set to TRUE, causes 4-state data bytes to be included */
  bool    net          /*!< If set to TRUE, causes default value to be written as Z instead of X */
) { PROFILE(VECTOR_DB_WRITE);

  uint8 mask;   /* Mask value for vector values */

  assert( vec != NULL );
  assert( vec->width > 0 );

  /* Calculate vector data mask */
  mask = write_data ? 0xff : 0xfc;
  switch( vec->suppl.part.type ) {
    case VTYPE_VAL :  mask = mask & 0x03;  break;
    case VTYPE_SIG :  mask = mask & 0x1b;  break;
    case VTYPE_EXP :  mask = mask & 0x3f;  break;
    case VTYPE_MEM :  mask = mask & 0x7b;  break;
    default        :  break;
  }

  /* Output vector information to specified file */
  /*@-formatcode@*/
  fprintf( file, "%u %hhu",
    vec->width,
    (vec->suppl.all & VSUPPL_MASK)
  );
  /*@=formatcode@*/

  /* Only write our data if we own it */
  if( vec->suppl.part.owns_data == 1 ) {

    /* Output value based on data type */
    switch( vec->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong        dflt_l = net ? UL_SET : 0x0;
          ulong        dflt_h = (vec->suppl.part.is_2state == 1) ? 0x0 : UL_SET;
          unsigned int i, j;
          ulong        hmask  = UL_HMASK( vec->width - 1 );
          for( i=0; i<(UL_SIZE(vec->width) - 1); i++ ) {
            fprintf( file, " %lx", (write_data && (vec->value.ul != NULL)) ? vec->value.ul[i][VTYPE_INDEX_VAL_VALL] : dflt_l );
            fprintf( file, " %lx", (write_data && (vec->value.ul != NULL)) ? vec->value.ul[i][VTYPE_INDEX_VAL_VALH] : dflt_h );
            for( j=2; j<vector_type_sizes[vec->suppl.part.type]; j++ ) {
              if( ((mask >> j) & 0x1) == 1 ) {
                fprintf( file, " %lx", (vec->value.ul != NULL) ? vec->value.ul[i][j] : 0 );
              } else {
                fprintf( file, " 0" );
              }
            }
          }
          fprintf( file, " %lx", ((write_data && (vec->value.ul != NULL)) ? vec->value.ul[i][VTYPE_INDEX_VAL_VALL] : dflt_l) & hmask );
          fprintf( file, " %lx", ((write_data && (vec->value.ul != NULL)) ? vec->value.ul[i][VTYPE_INDEX_VAL_VALH] : dflt_h) & hmask );
          for( j=2; j<vector_type_sizes[vec->suppl.part.type]; j++ ) {
            if( ((mask >> j) & 0x1) == 1 ) {
              fprintf( file, " %lx", (vec->value.ul != NULL) ? (vec->value.ul[i][j] & hmask) : 0 );
            } else {
              fprintf( file, " 0" );
            }
          }
        }
        break;
      case VDATA_R64 :
        if( vec->value.r64 != NULL ) {
          if( vec->value.r64->str != NULL ) {
            fprintf( file, " 1 %s", vec->value.r64->str );
          } else {
            fprintf( file, " 0 %f", vec->value.r64->val );
          }
        } else {
          fprintf( file, " 0 0.0" );
        }
        break;
      case VDATA_R32 :
        if( vec->value.r32 != NULL ) {
          if( vec->value.r32->str != NULL ) {
            fprintf( file, " 1 %s", vec->value.r32->str );
          } else {
            fprintf( file, " 0 %f", vec->value.r32->val );
          }
        } else {
          fprintf( file, " 0 0.0" );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw

 Creates a new vector structure, parses current file line for vector information
 and returns new vector structure to calling function.
*/
void vector_db_read(
  vector** vec,  /*!< Pointer to vector to create */
  char**   line  /*!< Pointer to line to parse for vector information */
) { PROFILE(VECTOR_DB_READ);

  unsigned int width;       /* Vector bit width */
  vsuppl       suppl;       /* Temporary supplemental value */
  int          chars_read;  /* Number of characters read */

  /* Read in vector information */
  /*@-formatcode@*/
  if( sscanf( *line, "%u %hhu%n", &width, &(suppl.all), &chars_read ) == 2 ) {
  /*@=formatcode@*/

    *line = *line + chars_read;

    /* Create new vector */
    *vec              = vector_create( width, suppl.part.type, suppl.part.data_type, TRUE );
    (*vec)->suppl.all = suppl.all;

    if( suppl.part.owns_data == 1 ) {

      Try {

        switch( suppl.part.data_type ) {
          case VDATA_UL :
            {
              unsigned int i, j;
              for( i=0; i<=((width-1)>>(info_suppl.part.vec_ul_size+3)); i++ ) {
                for( j=0; j<vector_type_sizes[suppl.part.type]; j++ ) {
                  /* If the CDD vector size and our size are the same, just do a direct read */
#if SIZEOF_LONG == 4
                  if( info_suppl.part.vec_ul_size == 2 ) {
#elif SIZEOF_LONG == 8
                  if( info_suppl.part.vec_ul_size == 3 ) {
#else
#error "Unsupported long size"
#endif
                    if( sscanf( *line, "%lx%n", &((*vec)->value.ul[i][j]), &chars_read ) == 1 ) {
                      *line += chars_read;
                    } else {
                      print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                      Throw 0;
                    }

#if SIZEOF_LONG == 8
                  /* If the CDD file size is 32-bit and we are 64-bit, store two elements to our one */
                  } else if( info_suppl.part.vec_ul_size == 2 ) {
                    uint32 val;
                    if( sscanf( *line, "%x%n", &val, &chars_read ) == 1 ) {
                      *line += chars_read;
                      if( i == 0 ) {
                        (*vec)->value.ul[i/2][j] = (ulong)val;
                      } else {
                        (*vec)->value.ul[i/2][j] |= ((ulong)val << 32);
                      }
                    } else {
                      print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                      printf( "vector Throw A.1\n" );
                      Throw 0;
                    }

#elif SIZEOF_LONG == 4
                  /* If the CDD file size is 64-bit and we are 32-bit, store one elements to our two */
                  } else if( info_suppl.part.vec_ul_size == 3 ) {
                    unsigned long long val;
                    /*@-duplicatequals +ignorequals@*/
                    if( sscanf( *line, "%llx%n", &val, &chars_read ) == 1 ) {
                    /*@=duplicatequals =ignorequals@*/
                      *line += chars_read;
                      (*vec)->value.ul[(i*2)+0][j] = (ulong)(val & 0xffffffffLL);
                      (*vec)->value.ul[(i*2)+1][j] = (ulong)((val >> 32) & 0xffffffffLL);
                    } else {
                      print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                      printf( "vector Throw A.2\n" );
                      Throw 0;
                    }
#endif
                  /* Otherwise, we don't know how to convert the value, so flag an error */
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                    Throw 0;
                  }
                }
              }
            }
            break;
          case VDATA_R64 :
            {
              int store_str;
              if( sscanf( *line, "%d%n", &store_str, &chars_read ) == 1 ) {
                *line += chars_read;
                if( store_str == 1 ) {
                  char str[4096];
                  if( sscanf( *line, "%s%n", str, &chars_read ) == 1 ) {
                    (*vec)->value.r64->str = strdup_safe( str );
                  }
                }
                if( sscanf( *line, "%lf%n", &((*vec)->value.r64->val), &chars_read ) == 1 ) {
                  *line += chars_read;
                } else {
                  print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                  Throw 0;
                }
              } else {
                print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                Throw 0;
              }
            }
            break;
          case VDATA_R32 :
            {
              int store_str;
              if( sscanf( *line, "%d%n", &store_str, &chars_read ) == 1 ) {
                *line += chars_read;
                if( store_str == 1 ) {
                  char str[4096];
                  if( sscanf( *line, "%s%n", str, &chars_read ) == 1 ) {
                    (*vec)->value.r32->str = strdup_safe( str );
                  }
                }
                if( sscanf( *line, "%f%n", &((*vec)->value.r32->val), &chars_read ) == 1 ) {
                  *line += chars_read;
                } else {
                  print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                  Throw 0;
                }
              } else {
                print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                Throw 0;
              }
            }
            break;
          default :  assert( 0 );  break;
        }

      } Catch_anonymous {
        vector_dealloc( *vec );
        *vec = NULL;
        Throw 0;
      }

    /* Otherwise, deallocate the vector data */
    } else {

      vector_dealloc_value( *vec );

    }

  } else {

    print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw

 Parses current file line for vector information and performs vector merge of 
 base vector and read vector information.  If the vectors are found to be different
 (width is not equal), an error message is sent to the user and the
 program is halted.  If the vectors are found to be equivalents, the merge is
 performed on the vector elements.
*/
void vector_db_merge(
  vector* base,  /*!< Base vector to merge data into */
  char**  line,  /*!< Pointer to line to parse for vector information */
  bool    same   /*!< Specifies if vector to merge needs to be exactly the same as the existing vector */
) { PROFILE(VECTOR_DB_MERGE);

  unsigned int width;       /* Width of read vector */
  vsuppl       suppl;       /* Supplemental value of vector */
  int          chars_read;  /* Number of characters read */

  assert( base != NULL );

  /*@-formatcode@*/
  if( sscanf( *line, "%u %hhu%n", &width, &(suppl.all), &chars_read ) == 2 ) {
  /*@=formatcode@*/

    *line = *line + chars_read;

    if( base->width != width ) {

      if( same ) {
        print_output( "Attempting to merge databases derived from different designs.  Unable to merge",
                      FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

    } else if( base->suppl.part.owns_data == 1 ) {

      switch( base->suppl.part.data_type ) {
        case VDATA_UL :
          {
            unsigned int i, j;
            for( i=0; i<=((width-1)>>(info_suppl.part.vec_ul_size+3)); i++ ) {
              for( j=0; j<vector_type_sizes[suppl.part.type]; j++ ) {

                /* If the CDD vector size and our size are the same, just do a direct read */
#if SIZEOF_LONG == 4
                if( info_suppl.part.vec_ul_size == 2 ) {
#elif SIZEOF_LONG == 8
                if( info_suppl.part.vec_ul_size == 3 ) {
#else
#error "Unsupported long size"
#endif
                  ulong val;
                  if( sscanf( *line, "%lx%n", &val, &chars_read ) == 1 ) {
                    *line += chars_read;
                    if( j >= 2 ) {
                      base->value.ul[i][j] |= val;
                    }
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
                    Throw 0;
                  }

#if SIZEOF_LONG == 8
                /* If the CDD file size is 32-bit and we are 64-bit, store two elements to our one */
                } else if( info_suppl.part.vec_ul_size == 2 ) {
                  uint32 val;
                  if( sscanf( *line, "%x%n", &val, &chars_read ) == 1 ) {
                    *line += chars_read;
                    if( j >= 2 ) {
                      if( i == 0 ) {
                        base->value.ul[i/2][j] = (ulong)val;
                      } else {
                        base->value.ul[i/2][j] |= ((ulong)val << 32);
                      }
                    }
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
                    printf( "vector Throw E.1\n" );
                    Throw 0;
                  }

#elif SIZEOF_LONG == 4
                /* If the CDD file size is 64-bit and we are 32-bit, store one elements to our two */
                } else if( info_suppl.part.vec_ul_size == 3 ) {
                  unsigned long long val;
                  /*@-duplicatequals +ignorequals@*/
                  if( sscanf( *line, "%llx%n", &val, &chars_read ) == 1 ) {
                  /*@=duplicatequals =ignorequals@*/
                    *line += chars_read;
                    if( j >= 2 ) {
                      base->value.ul[(i*2)+0][j] = (ulong)(val & 0xffffffffLL);
                      base->value.ul[(i*2)+1][j] = (ulong)((val >> 32) & 0xffffffffLL);
                    }
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
                    printf( "vector Throw E.2\n" );
                    Throw 0;
                  }
#endif
                /* Otherwise, we don't know how to convert the value, so flag an error */
                } else {
                  print_output( "Unable to parse vector information in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
                  Throw 0;
                }
              }
            }
          }
          break;
        case VDATA_R64 :
          {
            char   str[4096];
            double value;
            if( sscanf( *line, "%f%n", &value, &chars_read ) == 1 ) {
              *line += chars_read;
              if( sscanf( *line, "%s%n", str, &chars_read ) == 1 ) {
                *line += chars_read;
              }
            } else {
              print_output( "Unable to parse vector information in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
              Throw 0;
            }
          }
          break;
        case VDATA_R32 :
          {
            char   str[4096];
            double value;
            if( sscanf( *line, "%f%n", &value, &chars_read ) == 1 ) {
              *line += chars_read;
              if( sscanf( *line, "%s%n", str, &chars_read ) == 1 ) {
                *line += chars_read;
              }
            } else {
              print_output( "Unable to parse vector information in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
              Throw 0;
            }
          }
          break;
        default :  assert( 0 );  break;
      }

    }

  } else {

    print_output( "Unable to parse vector line from database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Merges two vectors, placing the result back into the base vector.  This function is used by the GUI for calculating
 module coverage.
*/
void vector_merge(
  vector* base,  /*!< Vector which will contain the merged results */
  vector* other  /*!< Vector which will merge its results into the base vector */
) { PROFILE(VECTOR_MERGE);

  unsigned int i, j;  /* Loop iterator */

  assert( base != NULL );
  assert( base->width == other->width );

  switch( base->suppl.part.data_type ) {
    case VDATA_UL :
      for( i=0; i<UL_SIZE(base->width); i++ ) {
        for( j=2; j<vector_type_sizes[base->suppl.part.type]; j++ ) {
          base->value.ul[i][j] |= other->value.ul[i][j];
        }
      }
      break;
    case VDATA_R64 :
      /* Nothing to do here */
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

}

/*!
 \return Returns eval_a coverage information for specified vector and bit position.
*/
int vector_get_eval_a(
  vector* vec,   /*!< Pointer to vector to get eval_a information from */
  int     index  /*!< Index to retrieve bit from */
) { PROFILE(VECTOR_GET_EVAL_A);

  int retval;  /* Return value for this function */

  assert( vec != NULL );
  assert( vec->suppl.part.type == VTYPE_EXP );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL  :  retval = (vec->value.ul[UL_DIV(index)][VTYPE_INDEX_EXP_EVAL_A] >> UL_MOD(index)) & 0x1;  break;
    case VDATA_R64 :  retval = 0;
    default        :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns eval_b coverage information for specified vector and bit position.
*/
int vector_get_eval_b(
  vector* vec,   /*!< Pointer to vector to get eval_b information from */
  int     index  /*!< Index to retrieve bit from */
) { PROFILE(VECTOR_GET_EVAL_B);

  int retval;  /* Return value for this function */

  assert( vec != NULL );
  assert( vec->suppl.part.type == VTYPE_EXP );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL  :  retval = (vec->value.ul[UL_DIV(index)][VTYPE_INDEX_EXP_EVAL_B] >> UL_MOD(index)) & 0x1;  break;
    case VDATA_R64 :  retval = 0;
    default        :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns eval_c coverage information for specified vector and bit position.
*/
int vector_get_eval_c(
  vector* vec,   /*!< Pointer to vector to get eval_c information from */
  int     index  /*!< Index to retrieve bit from */
) { PROFILE(VECTOR_GET_EVAL_C);

  int retval;  /* Return value for this function */

  assert( vec != NULL );
  assert( vec->suppl.part.type == VTYPE_EXP );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL  :  retval = (vec->value.ul[UL_DIV(index)][VTYPE_INDEX_EXP_EVAL_C] >> UL_MOD(index)) & 0x1;  break;
    case VDATA_R64 :  retval = 0;
    default        :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns eval_a coverage information for specified vector and bit position.
*/
int vector_get_eval_d(
  vector* vec,   /*!< Pointer to vector to get eval_d information from */
  int     index  /*!< Index to retrieve bit from */
) { PROFILE(VECTOR_GET_EVAL_D);

  int retval;  /* Return value for this function */

  assert( vec != NULL );
  assert( vec->suppl.part.type == VTYPE_EXP );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL  :  retval = (vec->value.ul[UL_DIV(index)][VTYPE_INDEX_EXP_EVAL_D] >> UL_MOD(index)) & 0x1;  break;
    case VDATA_R64 :  retval = 0;
    default        :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns the number of eval_a/b bits are set in the given vector.
*/
int vector_get_eval_ab_count(
  vector* vec  /*!< Pointer to vector to count eval_a/b bits in */
) { PROFILE(VECTOR_GET_EVAL_AB_COUNT);

  int          count = 0;  /* Number of EVAL_A/B bits set */
  unsigned int i, j;       /* Loop iterators */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      for( i=0; i<UL_SIZE( vec->width ); i++ ) {
        ulong value_a = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_A];
        ulong value_b = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_B];
        for( j=0; j<UL_BITS; j++ ) {
          count += (value_a >> j) & 0x1;
          count += (value_b >> j) & 0x1;
        }
      }
      break;
    case VDATA_R64 :
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( count );

}

/*!
 \return Returns the number of eval_a/b/c bits are set in the given vector.
*/
int vector_get_eval_abc_count(
  vector* vec  /*!< Pointer to vector to count eval_a/b/c bits in */
) { PROFILE(VECTOR_GET_EVAL_ABC_COUNT);

  int          count = 0;  /* Number of EVAL_A/B/C bits set */
  unsigned int i, j;       /* Loop iterators */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      for( i=0; i<UL_SIZE( vec->width ); i++ ) {
        ulong value_a = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_A]; 
        ulong value_b = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_B]; 
        ulong value_c = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_C]; 
        for( j=0; j<UL_BITS; j++ ) {
          count += (value_a >> j) & 0x1;
          count += (value_b >> j) & 0x1;
          count += (value_c >> j) & 0x1;
        }
      }
      break;
    case VDATA_R64 :
      break;
    default :  assert( 0 );  break;
  }
  
  PROFILE_END;

  return( count );

}

/*!
 \return Returns the number of eval_a/b/c/d bits are set in the given vector.
*/
int vector_get_eval_abcd_count(
  vector* vec  /*!< Pointer to vector to count eval_a/b/c/d bits in */
) { PROFILE(VECTOR_GET_EVAL_ABCD_COUNT);

  int          count = 0;  /* Number of EVAL_A/B/C/D bits set */
  unsigned int i, j;       /* Loop iterators */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      for( i=0; i<UL_SIZE( vec->width ); i++ ) {
        ulong value_a = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_A]; 
        ulong value_b = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_B]; 
        ulong value_c = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_C]; 
        ulong value_d = vec->value.ul[i][VTYPE_INDEX_EXP_EVAL_D]; 
        for( j=0; j<UL_BITS; j++ ) {
          count += (value_a >> j) & 0x1;
          count += (value_b >> j) & 0x1;
          count += (value_c >> j) & 0x1;
          count += (value_d >> j) & 0x1;
        }
      }
      break;
    case VDATA_R64 :
      break;
    default :  assert( 0 );  break;
  }
  
  PROFILE_END;

  return( count );

}

/*!
 \return Returns a string showing the toggle 0 -> 1 information.
*/
char* vector_get_toggle01_ulong(
  ulong** value,  /*!< Pointer to vector data array to get string from */
  int     width   /*!< Width of given vector data array */
) { PROFILE(VECTOR_GET_TOGGLE01_ULONG);

  char* bits = (char*)malloc_safe( width + 1 );
  int   i;
  char  tmp[2];

  for( i=width; i--; ) {
    /*@-formatcode@*/
    unsigned int rv = snprintf( tmp, 2, "%hhx", (unsigned char)((value[UL_DIV(i)][VTYPE_INDEX_SIG_TOG01] >> UL_MOD(i)) & 0x1) );
    /*@=formatcode@*/
    assert( rv < 2 );
    bits[i] = tmp[0];
  }

  bits[width] = '\0';
    
  PROFILE_END;

  return( bits );

}

/*!
 \return Returns a string showing the toggle 1 -> 0 information.
*/
char* vector_get_toggle10_ulong(
  ulong** value,  /*!< Pointer to vector data array to get string from */
  int     width   /*!< Width of given vector data array */
) { PROFILE(VECTOR_GET_TOGGLE10_ULONG);

  char* bits = (char*)malloc_safe( width + 1 );
  int   i;
  char  tmp[2];
  
  for( i=width; i--; ) {
    /*@-formatcode@*/ 
    unsigned int rv = snprintf( tmp, 2, "%hhx", (unsigned char)((value[UL_DIV(i)][VTYPE_INDEX_SIG_TOG10] >> UL_MOD(i)) & 0x1) );
    /*@=formatcode@*/ 
    assert( rv < 2 );
    bits[i] = tmp[0];
  } 
  
  bits[width] = '\0';

  PROFILE_END;

  return( bits );

}

/*!
 Displays the ulong toggle01 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle01_ulong(
  ulong** value,  /*!< Value array to display toggle information */
  int     width,  /*!< Number of bits in value array to display */
  FILE*   ofile   /*!< Stream to output information to */
) { PROFILE(VECTOR_DISPLAY_TOGGLE01_ULONG);

  unsigned int nib       = 0;
  int          i, j;
  int          bits_left = UL_MOD(width - 1);

  fprintf( ofile, "%d'h", width );

  for( i=UL_SIZE(width); i--; ) {
    for( j=bits_left; j>=0; j-- ) {
      nib |= (((value[i][VTYPE_INDEX_SIG_TOG01] >> (unsigned int)j) & 0x1) << ((unsigned int)j % 4));
      if( (j % 4) == 0 ) {
        fprintf( ofile, "%1x", nib );
        nib = 0;
      }
      if( ((j % 16) == 0) && ((j != 0) || (i != 0)) ) {
        fprintf( ofile, "_" );
      }
    }
    bits_left = (UL_BITS - 1);
  }

  PROFILE_END;

}

/*!
 Displays the ulong toggle10 information from the specified vector to the output
 stream specified in ofile.
*/
void vector_display_toggle10_ulong(
  ulong** value,  /*!< Value array to display toggle information */
  int     width,  /*!< Number of bits of value array to display */
  FILE*   ofile   /*!< Stream to output information to */
) { PROFILE(VECTOR_DISPLAY_TOGGLE10_ULONG);

  unsigned int nib       = 0;
  int          i, j;
  int          bits_left = UL_MOD(width - 1);
  
  fprintf( ofile, "%d'h", width );
      
  for( i=UL_SIZE(width); i--; ) {
    for( j=bits_left; j>=0; j-- ) {
      nib |= (((value[i][VTYPE_INDEX_SIG_TOG10] >> (unsigned int)j) & 0x1) << ((unsigned int)j % 4));
      if( (j % 4) == 0 ) {
        fprintf( ofile, "%1x", nib );
        nib = 0;
      }
      if( ((j % 16) == 0) && ((j != 0) || (i != 0)) ) {
        fprintf( ofile, "_" );
      }
    } 
    bits_left = (UL_BITS - 1);
  }

  PROFILE_END;

}

/*!
 Displays the binary value of the specified ulong vector data array to standard output.
*/
void vector_display_value_ulong(
  ulong** value,  /*!< Pointer to vector value array */
  int     width   /*!< Number of elements in value array */
) {

  int i, j;  /* Loop iterator */
  int bits_left = UL_MOD(width - 1);

  printf( "value: %d'b", width );

  for( i=UL_SIZE(width); i--; ) {
    for( j=bits_left; j>=0; j-- ) {
      if( ((value[i][VTYPE_INDEX_VAL_VALH] >> (unsigned int)j) & 0x1) == 0 ) {
        printf( "%lu", ((value[i][VTYPE_INDEX_VAL_VALL] >> (unsigned int)j) & 0x1) );
      } else {
        if( ((value[i][VTYPE_INDEX_VAL_VALL] >> (unsigned int)j) & 0x1) == 0 ) {
          printf( "x" );
        } else {
          printf( "z" );
        }
      }
    }
    bits_left = (UL_BITS - 1);
  }

}

/*!
 Outputs the specified ulong value array to standard output as described by the
 width parameter.
*/
void vector_display_ulong(
  ulong**      value,  /*!< Value array to display */
  unsigned int width,  /*!< Number of bits in array to display */
  unsigned int type   /*!< Type of vector to display */
) {

  unsigned int i, j;  /* Loop iterator */

  for( i=0; i<vector_type_sizes[type]; i++ ) {
    for( j=UL_SIZE(width); j--; ) {
      /*@-formatcode@*/
      printf( " %lx", value[j][i] );
      /*@=formatcode@*/
    }
  }

  /* Display value */
  printf( ", " );
  vector_display_value_ulong( value, width );

  switch( type ) {

    case VTYPE_SIG :

      /* Display toggle01 history */
      printf( ", 0->1: " );
      vector_display_toggle01_ulong( value, width, stdout );

      /* Display toggle10 history */
      printf( ", 1->0: " );
      vector_display_toggle10_ulong( value, width, stdout );

      break;

    case VTYPE_EXP :

      /* Display eval_a information */
      printf( ", a: %u'h", width );
      for( i=UL_SIZE(width); i--; ) {
        /*@-formatcode@*/
#if SIZEOF_LONG == 4
        printf( "%08lx", value[i][VTYPE_INDEX_EXP_EVAL_A] );
#elif SIZEOF_LONG == 8
        printf( "%016lx", value[i][VTYPE_INDEX_EXP_EVAL_A] );
#else
#error "Unsupported long size"
#endif
        /*@=formatcode@*/
      }

      /* Display eval_b information */
      printf( ", b: %u'h", width );
      for( i=UL_SIZE(width); i--; ) {
        /*@-formatcode@*/
#if SIZEOF_LONG == 4
        printf( "%08lx", value[i][VTYPE_INDEX_EXP_EVAL_B] );
#elif SIZEOF_LONG == 8
        printf( "%016lx", value[i][VTYPE_INDEX_EXP_EVAL_B] );
#endif
        /*@=formatcode@*/
      }

      /* Display eval_c information */
      printf( ", c: %u'h", width );
      for( i=UL_SIZE(width); i--; ) {
        /*@-formatcode@*/
#if SIZEOF_LONG == 4
        printf( "%08lx", value[i][VTYPE_INDEX_EXP_EVAL_C] );
#elif SIZEOF_LONG == 8
        printf( "%016lx", value[i][VTYPE_INDEX_EXP_EVAL_C] );
#endif
        /*@=formatcode@*/
      }

      /* Display eval_d information */
      printf( ", d: %u'h", width );
      for( i=UL_SIZE(width); i--; ) {
        /*@-formatcode@*/
#if SIZEOF_LONG == 4
        printf( "%08lx", value[i][VTYPE_INDEX_EXP_EVAL_D] );
#elif SIZEOF_LONG == 8
        printf( "%016lx", value[i][VTYPE_INDEX_EXP_EVAL_D] );
#endif
        /*@=formatcode@*/
      }

      break;

    case VTYPE_MEM :
  
      /* Display toggle01 history */
      printf( ", 0->1: " );
      vector_display_toggle01_ulong( value, width, stdout );

      /* Display toggle10 history */
      printf( ", 1->0: " );
      vector_display_toggle10_ulong( value, width, stdout );

      /* Write history */
      printf( ", wr: %u'h", width );
      for( i=UL_SIZE(width); i--; ) {
        /*@-formatcode@*/
#if SIZEOF_LONG == 4
        printf( "%08lx", value[i][VTYPE_INDEX_MEM_WR] );
#elif SIZEOF_LONG == 8
        printf( "%016lx", value[i][VTYPE_INDEX_MEM_WR] );
#endif
        /*@=formatcode@*/
      }

      /* Read history */
      printf( ", rd: %u'h", width );
      for( i=UL_SIZE(width); i--; ) {
        /*@-formatcode@*/
#if SIZEOF_LONG == 4
        printf( "%08lx", value[i][VTYPE_INDEX_MEM_RD] );
#elif SIZEOF_LONG == 8
        printf( "%016lx", value[i][VTYPE_INDEX_MEM_RD] );
#endif
        /*@=formatcode@*/
      }

      break;

    default : break;

  }

}

/*!
 Outputs the contents of a 64-bit real vector value.
*/
void vector_display_r64(
  rv64* value  /*!< Pointer to real64 structure from vector */
) {

  printf( "read value: %s, stored value: %f", value->str, value->val );

}

/*!
 Outputs contents of vector to standard output (for debugging purposes only).
*/
void vector_display(
  const vector* vec  /*!< Pointer to vector to output to standard output */
) {

  assert( vec != NULL );

  /*@-formatcode@*/
  printf( "Vector (%p) => width: %u, suppl: %hhx\n", vec, vec->width, vec->suppl.all );
  /*@=formatcode@*/

  if( (vec->width > 0) && (vec->value.ul != NULL) ) {
    switch( vec->suppl.part.data_type ) {
      case VDATA_UL  :  vector_display_ulong( vec->value.ul, vec->width, vec->suppl.part.type );  break;
      case VDATA_R64 :  vector_display_r64( vec->value.r64 );  break;
      default        :  assert( 0 );  break;
    }
  } else {
    printf( "NO DATA" );
  }

  printf( "\n" );

}

/*!
 Walks through specified vector counting the number of toggle01 bits that
 are set and the number of toggle10 bits that are set.  Adds these values
 to the contents of tog01_cnt and tog10_cnt.
*/
void vector_toggle_count(
            vector*       vec,        /*!< Pointer to vector to parse */
  /*@out@*/ unsigned int* tog01_cnt,  /*!< Number of bits in vector that toggled from 0 to 1 */
  /*@out@*/ unsigned int* tog10_cnt   /*!< Number of bits in vector that toggled from 1 to 0 */
) { PROFILE(VECTOR_TOGGLE_COUNT);

  if( (vec->suppl.part.type == VTYPE_SIG) || (vec->suppl.part.type == VTYPE_MEM) ) {

    unsigned int i, j;

    switch( vec->suppl.part.data_type ) {
      case VDATA_UL :
        for( i=0; i<UL_SIZE(vec->width); i++ ) {
          for( j=0; j<UL_BITS; j++ ) {
            *tog01_cnt += ((vec->value.ul[i][VTYPE_INDEX_SIG_TOG01] >> j) & 0x1);
            *tog10_cnt += ((vec->value.ul[i][VTYPE_INDEX_SIG_TOG10] >> j) & 0x1);
          }
        }
        break;
      case VDATA_R64 :
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

}

/*!
 Counts the number of bits that were written and read for the given memory
 vector.
*/
void vector_mem_rw_count(
  vector*       vec,     /*!< Pointer to vector to parse */
  int           lsb,     /*!< Least-significant bit of memory element to get information for */
  int           msb,     /*!< Most-significant bit of memory element to get information for */
  unsigned int* wr_cnt,  /*!< Pointer to number of bits in vector that were written */
  unsigned int* rd_cnt   /*!< Pointer to number of bits in vector that were read */
) { PROFILE(VECTOR_MEM_RW_COUNT);

  unsigned int i, j;  /* Loop iterator */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      { 
        ulong lmask = UL_LMASK(lsb);
        ulong hmask = UL_HMASK(msb);
        if( UL_DIV(lsb) == UL_DIV(msb) ) {
          lmask &= hmask;
        }
        for( i=UL_DIV(lsb); i<=UL_DIV(msb); i++ ) {
          ulong mask = (i == UL_DIV(lsb)) ? lmask : ((i == UL_DIV(msb)) ? hmask : UL_SET);
          ulong wr   = vec->value.ul[i][VTYPE_INDEX_MEM_WR] & mask;
          ulong rd   = vec->value.ul[i][VTYPE_INDEX_MEM_RD] & mask;
          for( j=0; j<UL_BITS; j++ ) {
            *wr_cnt += (wr >> j) & 0x1;
            *rd_cnt += (rd >> j) & 0x1;
          }
        }
      }
      break;
    case VDATA_R64 :
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if assigned bit that is being set to 1 in this function was
         found to be previously set; otherwise, returns FALSE.

 Sets the assigned supplemental bit for the given bit range in the given vector.  Called by
 race condition checker code.
*/
bool vector_set_assigned(
  vector* vec,  /*!< Pointer to vector value to set */
  int     msb,  /*!< Most-significant bit to set in vector value */
  int     lsb   /*!< Least-significant bit to set in vector value */
) { PROFILE(VECTOR_SET_ASSIGNED);

  bool prev_assigned = FALSE;  /* Specifies if any set bit was previously set */

  assert( vec != NULL );
  assert( ((msb - lsb) < 0) || ((unsigned int)(msb - lsb) < vec->width) );
  assert( vec->suppl.part.type == VTYPE_SIG );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong lmask     = UL_LMASK(lsb);
        ulong hmask     = UL_HMASK(msb);
        int   i         = UL_DIV(lsb);
        int   msb_index = UL_DIV(msb);
        if( i == msb_index ) {
          lmask &= hmask;
          prev_assigned = ((vec->value.ul[i][VTYPE_INDEX_SIG_MISC] & lmask) != 0);
          vec->value.ul[i][VTYPE_INDEX_SIG_MISC] |= lmask;
        } else {
          prev_assigned |= ((vec->value.ul[i][VTYPE_INDEX_SIG_MISC] & lmask) != 0);
          vec->value.ul[i][VTYPE_INDEX_SIG_MISC] |= lmask;
          for( i++; i<msb_index; i++ ) {
            prev_assigned = (vec->value.ul[i][VTYPE_INDEX_SIG_MISC] != 0);
            vec->value.ul[i][VTYPE_INDEX_SIG_MISC] |= UL_SET;
          }
          prev_assigned |= ((vec->value.ul[i][VTYPE_INDEX_SIG_MISC] & hmask) != 0);
          vec->value.ul[i][VTYPE_INDEX_SIG_MISC] |= hmask;
        }
      }
      break;
    case VDATA_R64 :
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( prev_assigned );

}

/*!
 \return Returns TRUE if the assigned value has changed; otherwise, returns FALSE.

 This function is called after a value has been stored in the SCRATCH arrays.  This
 function calculates the vector coverage information based on the vector type and performs the assignment
 from the SCRATCH array to the 
*/
bool vector_set_coverage_and_assign_ulong(
  vector*      vec,       /*!< Pointer to vector to calculate coverage metrics for and perform scratch -> actual assignment */
  const ulong* scratchl,  /*!< Pointer to scratch array containing new lower data */
  const ulong* scratchh,  /*!< Pointer to scratch array containing new upper data */
  int          lsb,       /*!< Least-significant bit to get coverage for */
  int          msb        /*!< Most-significant bit to get coverage for */
) { PROFILE(VECTOR_SET_COVERAGE_AND_ASSIGN);

  bool         changed = FALSE;          /* Set to TRUE if the assigned value has changed */
  unsigned int lindex  = UL_DIV(lsb);    /* Index of lowest array entry */
  unsigned int hindex  = UL_DIV(msb);    /* Index of highest array entry */
  ulong        lmask   = UL_LMASK(lsb);  /* Mask to be used in lower element */
  ulong        hmask   = UL_HMASK(msb);  /* Mask to be used in upper element */
  unsigned int i;                        /* Loop iterator */
  uint8        prev_set;                 /* Specifies if this vector value has previously been set */

  /* If the lindex and hindex are the same, set lmask to the AND of the high and low masks */
  if( lindex == hindex ) {
    lmask &= hmask;
  }

  switch( vec->suppl.part.type ) {
    case VTYPE_VAL :
      for( i=lindex; i<=hindex; i++ ) {
        ulong* tvall = &(vec->value.ul[i][VTYPE_INDEX_SIG_VALL]);
        ulong* tvalh = &(vec->value.ul[i][VTYPE_INDEX_SIG_VALH]);
        ulong  mask  = (i==lindex) ? lmask : (i==hindex ? hmask : UL_SET);
        *tvall = (*tvall & ~mask) | (scratchl[i] & mask);
        *tvalh = (*tvalh & ~mask) | (scratchh[i] & mask);
      }
      changed = TRUE;
      break;
    case VTYPE_SIG :
      prev_set = vec->suppl.part.set;
      for( i=lindex; i<=hindex; i++ ) {
        ulong* entry = vec->value.ul[i];
        ulong  mask  = (i==lindex) ? lmask : (i==hindex ? hmask : UL_SET);
        ulong  fvall = scratchl[i] & mask;
        ulong  fvalh = scratchh[i] & mask;
        ulong  tvall = entry[VTYPE_INDEX_SIG_VALL];
        ulong  tvalh = entry[VTYPE_INDEX_SIG_VALH];
        if( (fvall != (tvall & mask)) || (fvalh != (tvalh & mask)) ) {
          ulong tvalx = tvalh & ~tvall & entry[VTYPE_INDEX_SIG_MISC];
          ulong xval  = entry[VTYPE_INDEX_SIG_XHOLD];
          if( prev_set == 1 ) {
            entry[VTYPE_INDEX_SIG_TOG01] |= ((~tvalh & ~tvall) | (tvalx & ~xval)) & (~fvalh &  fvall) & mask;
            entry[VTYPE_INDEX_SIG_TOG10] |= ((~tvalh &  tvall) | (tvalx &  xval)) & (~fvalh & ~fvall) & mask;
          }
          entry[VTYPE_INDEX_SIG_VALL]  = (tvall & ~mask) | fvall;
          entry[VTYPE_INDEX_SIG_VALH]  = (tvalh & ~mask) | fvalh;
          entry[VTYPE_INDEX_SIG_XHOLD] = (xval  & ~mask) | (tvall & mask);
          entry[VTYPE_INDEX_SIG_MISC] |= ~fvalh & mask;
          changed = TRUE;
        }
      }
      break;
    case VTYPE_MEM :
      for( i=lindex; i<=hindex; i++ ) {
        ulong* entry = vec->value.ul[i];
        ulong  mask  = (i==lindex) ? lmask : (i==hindex ? hmask : UL_SET);
        ulong  fvall = scratchl[i] & mask;
        ulong  fvalh = scratchh[i] & mask;
        ulong  tvall = entry[VTYPE_INDEX_MEM_VALL];
        ulong  tvalh = entry[VTYPE_INDEX_MEM_VALH];
        if( (fvall != (tvall & mask)) || (fvalh != (tvalh & mask)) ) {
          ulong tvalx = tvalh & ~tvall & entry[VTYPE_INDEX_MEM_MISC];
          ulong xval  = entry[VTYPE_INDEX_MEM_XHOLD];
          entry[VTYPE_INDEX_MEM_TOG01] |= ((~tvalh & ~tvall) | (tvalx & ~xval)) & (~fvalh &  fvall) & mask;
          entry[VTYPE_INDEX_MEM_TOG10] |= ((~tvalh &  tvall) | (tvalx &  xval)) & (~fvalh & ~fvall) & mask;
          entry[VTYPE_INDEX_MEM_WR]    |= mask;
          entry[VTYPE_INDEX_MEM_VALL]   = (tvall & ~mask) | fvall;
          entry[VTYPE_INDEX_MEM_VALH]   = (tvalh & ~mask) | fvalh;
          entry[VTYPE_INDEX_MEM_XHOLD]  = (xval  & ~mask) | (tvall & mask);
          entry[VTYPE_INDEX_MEM_MISC]  |= ~fvalh & mask;
          changed = TRUE;
        }
      }
      break;
    case VTYPE_EXP :
      for( i=lindex; i<=hindex; i++ ) {
        ulong* entry = vec->value.ul[i];
        ulong  mask  = (i==lindex) ? lmask : (i==hindex ? hmask : UL_SET);
        ulong  fvall = scratchl[i] & mask;
        ulong  fvalh = scratchh[i] & mask;
        ulong  tvall = entry[VTYPE_INDEX_EXP_VALL];
        ulong  tvalh = entry[VTYPE_INDEX_EXP_VALH];
        if( (fvall != (tvall & mask)) || (fvalh != (tvalh & mask)) ) {
          entry[VTYPE_INDEX_EXP_VALL] = (tvall & ~mask) | fvall;
          entry[VTYPE_INDEX_EXP_VALH] = (tvalh & ~mask) | fvalh;
          changed = TRUE;
        }
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( changed );

}

/*!
 Calculates the lower and upper sign extension values for the given vector.
*/
inline static void vector_get_sign_extend_vector_ulong(
            const vector* vec,    /*!< Pointer to vector to get MSB value from */
  /*@out@*/ ulong*        signl,  /*!< Pointer to value that will contain the lower value vector */
  /*@out@*/ ulong*        signh   /*!< Pointer to value that will contain the upper value vector */
) { PROFILE(VECTOR_GET_SIGN_EXTEND_VECTOR_ULONG);

  ulong* entry     = vec->value.ul[UL_DIV(vec->width - 1)];
  ulong  last_mask = (ulong)1 << UL_MOD(vec->width - 1);

  *signl = ((entry[VTYPE_INDEX_VAL_VALL] & last_mask) != 0) ? UL_SET : 0;
  *signh = ((entry[VTYPE_INDEX_VAL_VALH] & last_mask) != 0) ? UL_SET : 0;

  PROFILE_END;

}

/*!
 Performs signedness bit fill for the given value arrays.
*/
static void vector_sign_extend_ulong(
  ulong* vall,   /*!< Pointer to lower value array to bit fill */
  ulong* valh,   /*!< Pointer to upper value array to bit fill */
  ulong  signl,  /*!< Sign-extension value for the lower value (call vector_get_sign_extend_vector_ulong) */
  ulong  signh,  /*!< Sign-extension value for the upper value (call vector_get_sign_extend_vector_ulong) */
  int    last,   /*!< Index of last bit in vall/h to evalulate for bit fill */
  int    width   /*!< Width of vall/h to fill */
) { PROFILE(VECTOR_SIGN_EXTEND_ULONG);

  /* If any special sign-extension is necessary, handle it now */
  if( (signl != 0) || (signh != 0) ) {
    unsigned int i    = UL_DIV(last + 1);
    unsigned int size = UL_SIZE( width );
    if( UL_MOD(last + 1) == 0 ) {
      vall[i] = signl;
      valh[i] = signh;
    } else {
      ulong fmask = UL_LMASK(last + 1);
      vall[i] |= signl & fmask;
      valh[i] |= signh & fmask;
    }
    for( i++; i<size; i++ ) {
      vall[i] = signl;
      valh[i] = signh;
    }
  }

  PROFILE_END;

}

/*!
 Performs a fast left-shift of 0-bit-aligned data in vector and stores the result in the vall/h
 value arrays.
*/
static void vector_lshift_ulong(
  const vector* vec,   /*!< Pointer to vector containing value that we want to left-shift */
  ulong*        vall,  /*!< Pointer to intermediate value array containing lower bits of shifted value */
  ulong*        valh,  /*!< Pointer to intermediate value array containing upper bits of shifted value */
  int           lsb,   /*!< LSB offset */
  int           msb    /*!< MSB offset */
) { PROFILE(VECTOR_LSHIFT_ULONG);

  unsigned int diff = UL_DIV(msb) - UL_DIV(vec->width - 1);

  if( UL_DIV(lsb) == UL_DIV(msb) ) {

    vall[diff] = (vec->value.ul[0][VTYPE_INDEX_VAL_VALL] << (unsigned int)lsb);
    valh[diff] = (vec->value.ul[0][VTYPE_INDEX_VAL_VALH] << (unsigned int)lsb);

  } else if( UL_MOD(lsb) == 0 ) {

    int i;

    for( i=UL_DIV(vec->width - 1); i>=0; i-- ) {
      vall[i+diff] = vec->value.ul[i][VTYPE_INDEX_VAL_VALL];
      valh[i+diff] = vec->value.ul[i][VTYPE_INDEX_VAL_VALH];
    }

    for( i=(UL_DIV(lsb) - 1); i>=0; i-- ) {
      vall[i] = 0;
      valh[i] = 0;
    }

  } else if( UL_MOD(msb) > UL_MOD(vec->width - 1) ) {

    unsigned int mask_bits1  = UL_MOD(vec->width);
    unsigned int shift_bits1 = UL_MOD(msb) - UL_MOD(vec->width - 1);
    ulong        mask1       = UL_SET >> (UL_BITS - mask_bits1);
    ulong        mask2       = UL_SET << (UL_BITS - shift_bits1);
    ulong        mask3       = ~mask2;
    int          i;

    vall[UL_DIV(msb)] = (vec->value.ul[UL_DIV(vec->width-1)][VTYPE_INDEX_VAL_VALL] & mask1) << shift_bits1;
    valh[UL_DIV(msb)] = (vec->value.ul[UL_DIV(vec->width-1)][VTYPE_INDEX_VAL_VALH] & mask1) << shift_bits1;

    for( i=(UL_DIV(vec->width - 1) - 1); i>=0; i-- ) {
      vall[i+diff+1] |= ((vec->value.ul[i][VTYPE_INDEX_VAL_VALL] & mask2) >> (UL_BITS - shift_bits1));
      valh[i+diff+1] |= ((vec->value.ul[i][VTYPE_INDEX_VAL_VALH] & mask2) >> (UL_BITS - shift_bits1));
      vall[i+diff]    = ((vec->value.ul[i][VTYPE_INDEX_VAL_VALL] & mask3) << shift_bits1);
      valh[i+diff]    = ((vec->value.ul[i][VTYPE_INDEX_VAL_VALH] & mask3) << shift_bits1);
    }

    for( i=(UL_DIV(lsb) - 1); i>=0; i-- ) {
      vall[i] = 0;
      valh[i] = 0;
    }

  } else {

    unsigned int mask_bits1  = UL_MOD(vec->width - 1);
    unsigned int shift_bits1 = mask_bits1 - UL_MOD(msb);
    ulong        mask1       = UL_SET << mask_bits1;
    ulong        mask2       = UL_SET >> (UL_BITS - shift_bits1);
    ulong        mask3       = ~mask2;
    int          i;

    vall[UL_DIV(msb)] = (vec->value.ul[UL_DIV(vec->width-1)][VTYPE_INDEX_VAL_VALL] & mask1) >> shift_bits1;
    valh[UL_DIV(msb)] = (vec->value.ul[UL_DIV(vec->width-1)][VTYPE_INDEX_VAL_VALH] & mask1) >> shift_bits1;

    for( i=UL_DIV(vec->width - 1); i>=0; i-- ) {
      vall[(i+diff)-1]  = ((vec->value.ul[i][VTYPE_INDEX_VAL_VALL] & mask2) << (UL_BITS - shift_bits1));
      valh[(i+diff)-1]  = ((vec->value.ul[i][VTYPE_INDEX_VAL_VALH] & mask2) << (UL_BITS - shift_bits1));
      if( i > 0 ) {
        vall[(i+diff)-1] |= ((vec->value.ul[i-1][VTYPE_INDEX_VAL_VALL] & mask3) >> shift_bits1);
        valh[(i+diff)-1] |= ((vec->value.ul[i-1][VTYPE_INDEX_VAL_VALH] & mask3) >> shift_bits1);
      }
    }

    for( i=(UL_DIV(lsb) - 1); i>=0; i-- ) {
      vall[i] = 0;
      valh[i] = 0;
    }

  }

  PROFILE_END;

}

/*!
 Performs a fast right-shift to zero-align data in vector and stores the result in the vall/h
 value arrays.
*/
static void vector_rshift_ulong(
  const vector* vec,   /*!< Pointer to vector containing value that we want to right-shift */
  ulong*        vall,  /*!< Pointer to intermediate value array containing lower bits of shifted value */
  ulong*        valh,  /*!< Pointer to intermediate value array containing upper bits of shifted value */
  int           lsb,   /*!< LSB of vec range to shift */
  int           msb    /*!< MSB of vec range to shift */
) { PROFILE(VECTOR_RSHIFT_ULONG);

  int          diff   = UL_DIV(lsb);
  unsigned int rwidth = (msb - lsb) + 1;

  if( lsb > msb ) {

    unsigned int i;

    for( i=0; i<UL_SIZE( vec->width ); i++ ) {
      vall[i] = 0;
      valh[i] = 0;
    }

  } else if( UL_DIV(lsb) == UL_DIV(msb) ) {

    unsigned int i;

    vall[0] = (vec->value.ul[diff][VTYPE_INDEX_VAL_VALL] >> UL_MOD(lsb));
    valh[0] = (vec->value.ul[diff][VTYPE_INDEX_VAL_VALH] >> UL_MOD(lsb));

    for( i=1; i<=UL_DIV(vec->width); i++ ) {
      vall[i] = 0;
      valh[i] = 0;
    }

  } else if( UL_MOD(lsb) == 0 ) {

    unsigned int i;
    ulong        lmask = UL_HMASK(msb);

    for( i=diff; i<UL_DIV(msb); i++ ) {
      vall[i-diff] = vec->value.ul[i][VTYPE_INDEX_VAL_VALL];
      valh[i-diff] = vec->value.ul[i][VTYPE_INDEX_VAL_VALH];
    }
    vall[i-diff] = vec->value.ul[i][VTYPE_INDEX_VAL_VALL] & lmask;
    valh[i-diff] = vec->value.ul[i][VTYPE_INDEX_VAL_VALH] & lmask;

    for( i=((i-diff)+1); i<UL_DIV(vec->width); i++ ) {
      vall[i] = 0;
      valh[i] = 0;
    }

  } else {

    unsigned int shift_bits = UL_MOD(lsb);
    ulong        mask1      = UL_SET << shift_bits;
    ulong        mask2      = ~mask1;
    ulong        mask3      = UL_SET >> ((UL_BITS - 1) - UL_MOD(msb - lsb));
    unsigned int hindex     = UL_DIV(vec->width - 1);
    unsigned int i;

    for( i=0; i<=UL_DIV(rwidth - 1); i++ ) {
      vall[i]  = (vec->value.ul[i+diff][VTYPE_INDEX_VAL_VALL] & mask1) >> shift_bits;
      valh[i]  = (vec->value.ul[i+diff][VTYPE_INDEX_VAL_VALH] & mask1) >> shift_bits;
      if( (i+diff+1) <= hindex ) {
        vall[i] |= (vec->value.ul[i+diff+1][VTYPE_INDEX_VAL_VALL] & mask2) << (UL_BITS - shift_bits);
        valh[i] |= (vec->value.ul[i+diff+1][VTYPE_INDEX_VAL_VALH] & mask2) << (UL_BITS - shift_bits);
      }
    }

    vall[i-1] &= mask3;
    valh[i-1] &= mask3;

    // Bit-fill with zeroes
    for( ; i<=hindex; i++ ) {
      vall[i] = 0;
      valh[i] = 0;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if assignment was performed; otherwise, returns FALSE.

 Allows the calling function to set any bit vector within the vector
 range.  If the vector value has never been set, sets
 the value to the new value and returns.  If the vector value has previously
 been set, checks to see if new vector bits have toggled, sets appropriate
 toggle values, sets the new value to this value and returns.
*/
bool vector_set_value_ulong(
  vector*      vec,    /*!< Pointer to vector to set value to */
  ulong**      value,  /*!< New value to set vector value to */
  unsigned int width   /*!< Width of new value */
) { PROFILE(VECTOR_SET_VALUE);

  bool  retval = FALSE;                   /* Return value for this function */
  int   i;                                /* Loop iterator */
  int   v2st;                             /* Value to AND with from value bit if the target is a 2-state value */
  ulong scratchl[UL_DIV(MAX_BIT_WIDTH)];  /* Lower scratch array */
  ulong scratchh[UL_DIV(MAX_BIT_WIDTH)];  /* Upper scratch array */

  assert( vec != NULL );

  /* Adjust the width if it exceeds our width */
  if( vec->width < width ) {
    width = vec->width;
  }

  /* Get some information from the vector */
  v2st = vec->suppl.part.is_2state << 1;

  /* Set upper bits to 0 */
  for( i=UL_DIV(vec->width - 1); (unsigned int)i>UL_DIV(width - 1); i-- ) {
    scratchl[i] = 0;
    scratchh[i] = 0;
  }

  /* Calculate the new values and place them in the scratch arrays */
  for( ; i>=0; i-- ) {
    scratchl[i] = v2st ? (~value[i][VTYPE_INDEX_VAL_VALH] & value[i][VTYPE_INDEX_VAL_VALL]) : value[i][VTYPE_INDEX_VAL_VALL];
    scratchh[i] = v2st ? 0 : value[i][VTYPE_INDEX_VAL_VALH];
  }

  /* Calculate the coverage and perform the actual assignment */
  retval = vector_set_coverage_and_assign_ulong( vec, scratchl, scratchh, 0, (vec->width - 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if stored data differed from original data; otherwise, returns FALSE.

 Used for single- and multi-bit part selection.  Bits are pulled from the source vector via the
 LSB and MSB range
*/
bool vector_part_select_pull(
  vector* tgt,        /*!< Pointer to vector that will store the result */
  vector* src,        /*!< Pointer to vector containing data to store */
  int     lsb,        /*!< LSB offset */
  int     msb,        /*!< MSB offset */
  bool    set_mem_rd  /*!< If TRUE, set the memory read bit in the source */
) { PROFILE(VECTOR_PART_SELECT_PULL);

  bool retval;  /* Return value for this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong valh[UL_DIV(MAX_BIT_WIDTH)];
        ulong vall[UL_DIV(MAX_BIT_WIDTH)];

        /* Perform shift operation */
        vector_rshift_ulong( src, vall, valh, lsb, msb );

        /* If the src vector is of type MEM, set the MEM_RD bit in the source's supplemental field */
        if( set_mem_rd && (src->suppl.part.type == VTYPE_MEM) ) {
          src->value.ul[UL_DIV(lsb)][VTYPE_INDEX_MEM_RD] |= ((ulong)1 << UL_MOD(lsb));
        }

        retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if stored data differed from original data; otherwise, returns FALSE.

 Used for single- and multi-bit part selection.  Bits are pushed from the source vector via the
 LSB and MSB range.
*/
bool vector_part_select_push(
  vector*       tgt,         /*!< Pointer to vector that will store the result */
  int           tgt_lsb,     /*!< LSB offset of target vector */
  int           tgt_msb,     /*!< MSB offset of target vector */
  const vector* src,         /*!< Pointer to vector containing data to store */
  int           src_lsb,     /*!< LSB offset of source vector */
  int           src_msb,     /*!< MSB offset of source vector */
  bool          sign_extend  /*!< Set to TRUE if sign extension is needed */
) { PROFILE(VECTOR_PART_SELECT_PUSH);

  bool retval;  /* Return value for this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
        ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int diff;
        unsigned int i; 
        ulong        signl, signh;

        /* Get the sign extension vector */
        vector_get_sign_extend_vector_ulong( src, &signl, &signh );

        /* If the LSB to assign exceeds the size of the actual vector, just create a value based on the signedness */
        if( (src_lsb > 0) && ((unsigned int)src_lsb >= src->width) ) {

          if( sign_extend && ((signl != 0) || (signh != 0)) ) {
            vector_sign_extend_ulong( vall, valh, signl, signh, (tgt_lsb - 1), tgt->width );
          } else {
            for( i=UL_DIV(tgt_lsb); i<=UL_DIV(tgt_msb); i++ ) {
              vall[i] = valh[i] = 0;
            }
          }

        /* Otherwise, pull the value from source vector */
        } else {

          /* First, initialize the vall/h arrays to zero */
          for( i=UL_DIV(tgt_lsb); i<=UL_DIV(tgt_msb); i++ ) {
            vall[i] = valh[i] = 0;
          }
  
          /* Left-shift the source vector to match up with target LSB */
          if( src_lsb < tgt_lsb ) {
            diff = (tgt_lsb - src_lsb);
            vector_lshift_ulong( src, vall, valh, diff, ((src_msb - src_lsb) + diff) );
          /* Otherwise, right-shift the source vector to match up */
          } else {
            diff = (src_lsb - tgt_lsb);
            vector_rshift_ulong( src, vall, valh, diff, ((src_msb - src_lsb) + diff) );
          }

          /* Now apply the sign extension, if necessary */
          if( sign_extend && ((signl != 0) || (signh != 0)) ) {
            vector_sign_extend_ulong( vall, valh, signl, signh, (tgt_lsb + (src_msb - src_lsb)), (tgt_msb + 1) );
          }

        }

        /* Now assign the calculated value and set coverage information */
        retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, tgt_lsb, tgt_msb );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 Called by expression_op_func__* functions for operations that are unary
 in nature.
*/
void vector_set_unary_evals(
  vector* vec  /*!< Pointer to vector to set eval_a/b bits for unary operation */
) { PROFILE(VECTOR_SET_UNARY_EVALS);

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i;
        unsigned int size = UL_SIZE(vec->width);
        for( i=0; i<size; i++ ) {
          ulong* entry = vec->value.ul[i];
          ulong  lval  =  entry[VTYPE_INDEX_EXP_VALL];
          ulong  nhval = ~entry[VTYPE_INDEX_EXP_VALH];
          entry[VTYPE_INDEX_EXP_EVAL_A] |= nhval & ~lval;
          entry[VTYPE_INDEX_EXP_EVAL_B] |= nhval &  lval;
        }
      }
      break;
    case VDATA_R64 :
    case VDATA_R32 :
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

}

/*!
 Sets the eval_a/b/c supplemental bits as necessary.  This function should be called
 by expression_op_func__* functions that are AND-type combinational operations only and own their
 own vectors.
*/    
void vector_set_and_comb_evals(
  vector* tgt,   /*!< Pointer to target vector to set eval_a/b/c supplemental bits */
  vector* left,  /*!< Pointer to target vector on the left */
  vector* right  /*!< Pointer to target vector on the right */
) { PROFILE(VECTOR_SET_AND_COMB_EVALS);

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i;
        unsigned int size  = UL_SIZE( tgt->width );
        unsigned int lsize = UL_SIZE( left->width );
        unsigned int rsize = UL_SIZE( right->width );

        for( i=0; i<size; i++ ) {
          ulong* val    = tgt->value.ul[i];
          ulong* lval   = (i < lsize) ? left->value.ul[i]  : 0;
          ulong* rval   = (i < rsize) ? right->value.ul[i] : 0;
          ulong  lvall  = (i < lsize) ?  lval[VTYPE_INDEX_EXP_VALL] : 0;
          ulong  nlvalh = (i < lsize) ? ~lval[VTYPE_INDEX_EXP_VALH] : UL_SET;
          ulong  rvall  = (i < rsize) ?  rval[VTYPE_INDEX_EXP_VALL] : 0;
          ulong  nrvalh = (i < rsize) ? ~rval[VTYPE_INDEX_EXP_VALH] : UL_SET;
          
          val[VTYPE_INDEX_EXP_EVAL_A] |= nlvalh & ~lvall;
          val[VTYPE_INDEX_EXP_EVAL_B] |= nrvalh & ~rvall;
          val[VTYPE_INDEX_EXP_EVAL_C] |= nlvalh & nrvalh & lvall & rvall;
        }
      }
      break;
    case VDATA_R64 :
    case VDATA_R32 :
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;
 
}

/*!
 Sets the eval_a/b/c supplemental bits as necessary.  This function should be called
 by expression_op_func__* functions that are OR-type combinational operations only and own their
 own vectors.
*/
void vector_set_or_comb_evals(
  vector* tgt,   /*!< Pointer to target vector to set eval_a/b/c supplemental bits */
  vector* left,  /*!< Pointer to vector on left of expression */
  vector* right  /*!< Pointer to vector on right of expression */
) { PROFILE(VECTOR_SET_OR_COMB_EVALS);

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i;
        unsigned int size  = UL_SIZE( tgt->width );
        unsigned int lsize = UL_SIZE( left->width );
        unsigned int rsize = UL_SIZE( right->width );

        for( i=0; i<size; i++ ) {
          ulong* val    = tgt->value.ul[i];
          ulong* lval   = (i < lsize) ? left->value.ul[i]  : 0;
          ulong* rval   = (i < rsize) ? right->value.ul[i] : 0;
          ulong  lvall  = (i < lsize) ?  lval[VTYPE_INDEX_EXP_VALL] : 0;
          ulong  nlvalh = (i < lsize) ? ~lval[VTYPE_INDEX_EXP_VALH] : UL_SET;
          ulong  rvall  = (i < rsize) ?  rval[VTYPE_INDEX_EXP_VALL] : 0;
          ulong  nrvalh = (i < rsize) ? ~rval[VTYPE_INDEX_EXP_VALH] : UL_SET;

          val[VTYPE_INDEX_EXP_EVAL_A] |= nlvalh & lvall;
          val[VTYPE_INDEX_EXP_EVAL_B] |= nrvalh & rvall;
          val[VTYPE_INDEX_EXP_EVAL_C] |= nlvalh & nrvalh & ~lvall & ~rvall;
        }
      }
      break;
    case VDATA_R64 :
    case VDATA_R32 :
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

}

/*! 
 Sets the eval_a/b/c/d supplemental bits as necessary.  This function should be called
 by expression_op_func__* functions that are OTHER-type combinational operations only and own their
 own vectors. 
*/
void vector_set_other_comb_evals(
  vector* tgt,   /*!< Pointer to target vector to set eval_a/b/c/d supplemental bits */
  vector* left,  /*!< Pointer to vector on left of expression */
  vector* right  /*!< Pointer to vector on right of expression */
) { PROFILE(VECTOR_SET_OTHER_COMB_EVALS);

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i;
        unsigned int size  = UL_SIZE( tgt->width );
        unsigned int lsize = UL_SIZE( left->width );
        unsigned int rsize = UL_SIZE( right->width );

        for( i=0; i<size; i++ ) { 
          ulong* val    = tgt->value.ul[i];
          ulong* lval   = (i < lsize) ? left->value.ul[i]  : 0;
          ulong* rval   = (i < rsize) ? right->value.ul[i] : 0;
          ulong  lvall  = (i < lsize) ?  lval[VTYPE_INDEX_EXP_VALL] : 0;
          ulong  nlvalh = (i < lsize) ? ~lval[VTYPE_INDEX_EXP_VALH] : UL_SET;
          ulong  rvall  = (i < rsize) ?  rval[VTYPE_INDEX_EXP_VALL] : 0;
          ulong  nrvalh = (i < rsize) ? ~rval[VTYPE_INDEX_EXP_VALH] : UL_SET;
          ulong  nvalh  = nlvalh & nrvalh;

          val[VTYPE_INDEX_EXP_EVAL_A] |= nvalh & ~lvall & ~rvall;
          val[VTYPE_INDEX_EXP_EVAL_B] |= nvalh & ~lvall &  rvall;
          val[VTYPE_INDEX_EXP_EVAL_C] |= nvalh &  lvall & ~rvall;
          val[VTYPE_INDEX_EXP_EVAL_D] |= nvalh &  lvall &  rvall;
        }
      }
      break;
    case VDATA_R64 :
    case VDATA_R32 :
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;
  
} 

/*!
 \return Returns TRUE if the given vector contains unknown (X or Z) bits; otherwise, returns FALSE.
*/
bool vector_is_unknown(
  const vector* vec  /*!< Pointer to vector check for unknown-ness */
) { PROFILE(VECTOR_IS_UKNOWN);

  unsigned int i = 0;  /* Loop iterator */
  unsigned int size;   /* Size of data array */

  assert( vec != NULL );
  assert( vec->value.ul != NULL );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      size = UL_SIZE( vec->width );
      while( (i < size) && (vec->value.ul[i][VTYPE_INDEX_VAL_VALH] == 0) ) i++;
      break;
    case VDATA_R64 :
    case VDATA_R32 :
      size = 0;
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( i < size );

}

/*!
 \return Returns TRUE if the given vector contains at least one non-zero bit; otherwise, returns FALSE.
*/
bool vector_is_not_zero(
  const vector* vec  /*!< Pointer to vector to check for non-zero-ness */
) { PROFILE(VECTOR_IS_NOT_ZERO);

  unsigned int i = 0;  /* Loop iterator */
  unsigned int size;   /* Size of data array */

  assert( vec != NULL );
  assert( vec->value.ul != NULL );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      size = UL_SIZE( vec->width );
      while( (i < size) && (vec->value.ul[i][VTYPE_INDEX_VAL_VALL] == 0) ) i++;
      break;
    case VDATA_R64 :
      size = (vec->value.r64->val == 0.0) ? 1 : 0;
      break;
    case VDATA_R32 :
      size = (vec->value.r32->val == 0.0) ? 1 : 0;
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( i < size );

}

/*!
 \return Returns TRUE if the given vector changed value; otherwise, returns FALSE.

 Sets the entire specified vector to a value of X.
*/
bool vector_set_to_x(
  vector* vec  /*!< Pointer to vector to set to a value of X */
) { PROFILE(VECTOR_SET_TO_X);

  bool retval;  /* Return value for this function */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL:
      {
        ulong        scratchl[UL_DIV(MAX_BIT_WIDTH)];
        ulong        scratchh[UL_DIV(MAX_BIT_WIDTH)];
        ulong        end_mask = UL_HMASK(vec->width - 1);
        unsigned int i;
        for( i=0; i<(UL_SIZE(vec->width) - 1); i++ ) {
          scratchl[i] = 0;
          scratchh[i] = UL_SET;
        }
        scratchl[i] = 0;
        scratchh[i] = end_mask;
        retval = vector_set_coverage_and_assign_ulong( vec, scratchl, scratchh, 0, (vec->width - 1) );
      }
      break;
    case VDATA_R32 :
    case VDATA_R64 :
      retval = FALSE;
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns integer value of specified vector.

 Converts a vector structure into an integer value.  If the number of bits for the
 vector exceeds the number of bits in an integer, the upper bits of the vector are
 unused.
*/
int vector_to_int(
  const vector* vec  /*!< Pointer to vector to convert into integer */
) { PROFILE(VECTOR_TO_INT);

  int          retval;  /* Integer value returned to calling function */
  unsigned int width = (vec->width > (sizeof( int ) * 8)) ? (sizeof( int ) * 8) : vec->width;

  assert( width > 0 );

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL  :  retval = vec->value.ul[0][VTYPE_INDEX_VAL_VALL];  break;
    case VDATA_R64 :  retval = (int)round( vec->value.r64->val );
    case VDATA_R32 :  retval = (int)roundf( vec->value.r32->val );
    default        :  assert( 0 );  break;
  }

  /* If the vector is signed, sign-extend the integer */
  if( (vec->suppl.part.is_signed == 1) && (width < (sizeof( int ) * 8)) ) {
    /*@-shiftimplementation@*/
    retval |= ((UL_SET * ((retval >> (width - 1)) & 0x1)) << width);
    /*@=shiftimplementation@*/
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns integer value of specified vector.

 Converts a vector structure into an integer value.  If the number of bits for the
 vector exceeds the number of bits in an integer, the upper bits of the vector are
 unused.
*/
uint64 vector_to_uint64(
  const vector* vec  /*!< Pointer to vector to convert into integer */
) { PROFILE(VECTOR_TO_UINT64);

  uint64 retval = 0;   /* 64-bit integer value returned to calling function */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      if( (vec->width > 32) && (sizeof( ulong ) == 32) ) {
        retval = ((uint64)vec->value.ul[1][VTYPE_INDEX_VAL_VALL] << 32) | (uint64)vec->value.ul[0][VTYPE_INDEX_VAL_VALL];
      } else {
        retval = (uint64)vec->value.ul[0][VTYPE_INDEX_VAL_VALL];
      }
      break;
    case VDATA_R64 :
      retval = (uint64)round( vec->value.r64->val );
      break;
    case VDATA_R32 :
      retval = (uint64)roundf( vec->value.r32->val );
      break;
    default :  assert( 0 );  break;
  }

  /* If the vector is signed, sign-extend the integer */
  if( vec->suppl.part.is_signed == 1 ) {
    unsigned int width = (vec->width > 64) ? 64 : vec->width;
    retval |= (UINT64(0xffffffffffffffff) * ((retval >> (width - 1)) & 0x1)) << width;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns 64-bit real value version of the specified vector.

 Converts the specified vector to a 64-bit real number.  If the value exceeds what can be stored
 in a 64-bit value, the upper bits are dropped.
*/
real64 vector_to_real64(
  const vector* vec  /*!< Pointer to vector to convert into a 64-bit real value */
) { PROFILE(VECTOR_TO_REAL64);

  real64 retval;  /* Return value for this function */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL  :  retval = (double)vector_to_uint64( vec );  break;
    case VDATA_R64 :  retval = vec->value.r64->val;              break;
    case VDATA_R32 :  retval = (double)vec->value.r32->val;      break;
    default        :  assert( 0 );                               break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 Converts a vector structure into a sim_time structure.  If the number of bits for the
 vector exceeds the number of bits in an 64-bit integer, the upper bits of the vector are
 unused.
*/
void vector_to_sim_time(
            const vector* vec,    /*!< Pointer to vector to convert into integer */
            uint64        scale,  /*!< Scaling factor */
  /*@out@*/ sim_time*     time    /*!< Pointer to sim_time structure to populate */
) { PROFILE(VECTOR_TO_SIM_TIME);

  union {
    struct {
      uint32 lo;
      uint32 hi;
    } u32;
    uint64 full;
  } time_u = {0};

  /* Calculate the full (64-bit) time value */
  switch( vec->suppl.part.data_type ) {
    case VDATA_UL  :
      assert( vec->value.ul[0][VTYPE_INDEX_VAL_VALH] == 0 );
#if SIZEOF_LONG == 4
      time_u.u32.lo = vec->value.ul[0][VTYPE_INDEX_VAL_VALL];
      if( UL_SIZE( vec->width ) > 1 ) {
        assert( vec->value.ul[1][VTYPE_INDEX_VAL_VALH] == 0 );
        time_u.u32.hi = vec->value.ul[1][VTYPE_INDEX_VAL_VALL];
      }
#elif SIZEOF_LONG == 8
      time_u.full = vec->value.ul[0][VTYPE_INDEX_VAL_VALL];
#else
#error "Unsupported long size"
#endif
      time_u.full *= scale;
      break;
    case VDATA_R64 :  time_u.full = (uint64)round( vec->value.r64->val * scale );  break;
    case VDATA_R32 :  time_u.full = (uint64)roundf( vec->value.r32->val * scale );  break;
    default        :  assert( 0 );  break;
  }

  time->lo   = time_u.u32.lo;
  time->hi   = time_u.u32.hi;
  time->full = time_u.full;

  PROFILE_END;

}

/*!
 Converts an integer value into a vector, creating a vector value to store the
 new vector into.  This function is used along with the vector_to_int for
 mathematical vector operations.  We will first convert vectors into integers,
 perform the mathematical operation, and then revert the integers back into
 the vectors.
*/
void vector_from_int(
  vector* vec,   /*!< Pointer to vector store value into */
  int     value  /*!< Integer value to convert into vector */
) { PROFILE(VECTOR_FROM_INT);

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
#if UL_BITS < (SIZEOF_INT * 8)
        unsigned int i;
        unsigned int size = UL_SIZE( vec->width );
        for( i=0; i<size; i++ ) {
          vec->value.ul[i][VTYPE_INDEX_VAL_VALL] = (ulong)value & UL_SET;
          vec->value.ul[i][VTYPE_INDEX_VAL_VALH] = 0;
          value >>= UL_BITS;
        }
#else
        vec->value.ul[0][VTYPE_INDEX_VAL_VALL] = (ulong)value;
        vec->value.ul[0][VTYPE_INDEX_VAL_VALH] = 0;
#endif
      }
      break;
    case VDATA_R32 :
      vec->value.r32->val = (float)value;
      break;
    case VDATA_R64 :
      vec->value.r64->val = (double)value;
      break;
    default :  assert( 0 );  break;
  }

  /* Because this value came from an integer, specify that the vector is signed */
  vec->suppl.part.is_signed = 1;

  PROFILE_END;

}

/*!
 Converts a 64-bit integer value into a vector.  This function is used along with
 the vector_to_uint64 for mathematical vector operations.  We will first convert
 vectors into 64-bit integers, perform the mathematical operation, and then revert
 the 64-bit integers back into the vectors.
*/
void vector_from_uint64(
  vector* vec,   /*!< Pointer to vector store value into */
  uint64  value  /*!< 64-bit integer value to convert into vector */
) { PROFILE(VECTOR_FROM_UINT64);

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
#if UL_BITS < 64
        unsigned int i;
        unsigned int size = UL_SIZE( vec->width );
        for( i=0; i<size; i++ ) {
          vec->value.ul[i][VTYPE_INDEX_VAL_VALL] = (ulong)value & UL_SET;
          vec->value.ul[i][VTYPE_INDEX_VAL_VALH] = 0;
          value >>= UL_BITS;
        }
#else
        vec->value.ul[0][VTYPE_INDEX_VAL_VALL] = (ulong)value;
        vec->value.ul[0][VTYPE_INDEX_VAL_VALH] = 0;
#endif
      }
      break;
    case VDATA_R64 :
      vec->value.r64->val = (double)value;
      break;
    case VDATA_R32 :
      vec->value.r32->val = (float)value;
      break;
    default :  assert( 0 );  break;
  }

  /* Because this value came from an unsigned integer, specify that the vector is unsigned */
  vec->suppl.part.is_signed = 0;

  PROFILE_END;

}

/*!
 Iterates through string str starting at the left-most character, calculates the int value
 of the character and sets the appropriate number of bits in the specified vector locations.
*/
static void vector_set_static(
  vector*      vec,           /*!< Pointer to vector to add static value to */
  char*        str,           /*!< Value string to add */
  unsigned int bits_per_char  /*!< Number of bits necessary to store a value character (1, 3, or 4) */
) { PROFILE(VECTOR_SET_STATIC);

  char*        ptr       = str + (strlen( str ) - 1);  /* Pointer to current character evaluating */
  unsigned int pos       = 0;                          /* Current bit position in vector */
  unsigned int i;                                      /* Loop iterator */
  int          data_type = vec->suppl.part.data_type;  /* Copy of data type for performance reasons */

  while( ptr >= str ) {
    if( *ptr != '_' ) {
      if( (*ptr == 'x') || (*ptr == 'X') ) {
        switch( data_type ) {
          case VDATA_UL :
            for( i=0; i<bits_per_char; i++ ) {
              if( (i + pos) < vec->width ) {
                vec->value.ul[UL_DIV(i+pos)][VTYPE_INDEX_VAL_VALH] |= ((ulong)1 << UL_MOD(i+pos));
              }
            }
            break;
          default :  assert( 0 );  break;
        }
      } else if( (*ptr == 'z') || (*ptr == 'Z') || (*ptr == '?') ) {
        switch( data_type ) {
          case VDATA_UL :
            for( i=0; i<bits_per_char; i++ ) {
              if( (i + pos) < vec->width ) {
                unsigned int index = UL_DIV(i + pos);
                ulong        value = ((ulong)1 << UL_MOD(i + pos));
                vec->value.ul[index][VTYPE_INDEX_VAL_VALL] |= value;
                vec->value.ul[index][VTYPE_INDEX_VAL_VALH] |= value;
              }
            }
            break;
          default :  assert( 0 );  break;
        }
      } else {
        ulong val;
        if( (*ptr >= 'a') && (*ptr <= 'f') ) {
          val = (*ptr - 'a') + 10;
        } else if( (*ptr >= 'A') && (*ptr <= 'F') ) {
          val = (*ptr - 'A') + 10;
        } else {
          assert( (*ptr >= '0') && (*ptr <= '9') );
	  val = *ptr - '0';
        }
        switch( data_type ) {
          case VDATA_UL :
            for( i=0; i<bits_per_char; i++ ) {
              if( (i + pos) < vec->width ) {
                vec->value.ul[UL_DIV(i+pos)][VTYPE_INDEX_VAL_VALL] |= ((val >> i) & 0x1) << UL_MOD(i + pos);
              }
            }
            break;
          default :  assert( 0 );  break;
        }
      }
      pos = pos + bits_per_char;
    }
    ptr--;
  }

  /* Bit-fill, if necessary */
  if( pos < vec->width ) {
    switch( data_type ) {
      case VDATA_UL :
        {
          ulong hfill = (vec->value.ul[UL_DIV(pos)][VTYPE_INDEX_VAL_VALH] & ((ulong)1 << UL_MOD(pos - 1))) ? UL_SET : 0x0;
          ulong lfill = (vec->value.ul[UL_DIV(pos)][VTYPE_INDEX_VAL_VALL] & ((ulong)1 << UL_MOD(pos - 1))) ? hfill  : 0x0;
          ulong lmask = UL_LMASK(pos);
          ulong hmask = UL_HMASK(vec->width - 1);
          if( UL_DIV(pos) == UL_DIV(vec->width - 1) ) {
            ulong mask = lmask & hmask;
            vec->value.ul[UL_DIV(pos)][VTYPE_INDEX_VAL_VALL] |= lfill & mask;
            vec->value.ul[UL_DIV(pos)][VTYPE_INDEX_VAL_VALH] |= hfill & mask;
          } else {
            vec->value.ul[UL_DIV(pos)][VTYPE_INDEX_VAL_VALL] |= lfill & lmask;
            vec->value.ul[UL_DIV(pos)][VTYPE_INDEX_VAL_VALH] |= hfill & lmask;
            for( i=(UL_DIV(pos) + 1); i<(UL_SIZE( vec->width ) - 1); i++ ) {
              vec->value.ul[i][VTYPE_INDEX_VAL_VALL] = lfill;
              vec->value.ul[i][VTYPE_INDEX_VAL_VALH] = hfill;
            }
            vec->value.ul[i][VTYPE_INDEX_VAL_VALL] = lfill & hmask;
            vec->value.ul[i][VTYPE_INDEX_VAL_VALH] = hfill & hmask;
          }
        }
        break;
      default :  assert( 0 );
    }
  }

  PROFILE_END;

}

/*!
 \return Returns pointer to the allocated/coverted string.

 Converts a vector value into a string, allocating the memory for the string in this
 function and returning a pointer to that string.  The type specifies what type of
 value to change vector into.
*/
char* vector_to_string(
  vector* vec,      /*!< Pointer to vector to convert */
  int     base,     /*!< Base type of vector value */
  bool    show_all  /*!< Set to TRUE causes all bits in vector to be displayed (otherwise, only significant bits are displayed) */
) { PROFILE(VECTOR_TO_STRING);

  char* str = NULL;  /* Pointer to allocated string */

  if( base == QSTRING ) {

    int i, j;
    int vec_size = ((vec->width - 1) >> 3) + 2;
    int pos      = 0;

    /* Allocate memory for string from the heap */
    str = (char*)malloc_safe( vec_size );

    switch( vec->suppl.part.data_type ) {
      case VDATA_UL :
        {
          int offset = (((vec->width >> 3) & (UL_MOD_VAL >> 3)) == 0) ? SIZEOF_LONG : ((vec->width >> 3) & (UL_MOD_VAL >> 3));
          for( i=UL_SIZE(vec->width); i--; ) {
            ulong val = vec->value.ul[i][VTYPE_INDEX_VAL_VALL]; 
            for( j=(offset - 1); j>=0; j-- ) {
              str[pos] = (val >> ((unsigned int)j * 8)) & 0xff;
              pos++;
            }
            offset = SIZEOF_LONG;
          }
        }
        break;
      case VDATA_R64 :
        assert( 0 );
        break;
      default :  assert( 0 );  break;
    }

    str[pos] = '\0';

  } else if( base == DECIMAL ) {

    char         width_str[20];
    unsigned int rv = snprintf( width_str, 20, "%d", vector_to_int( vec ) );
    assert( rv < 20 );
    str = strdup_safe( width_str );

  } else if( vec->suppl.part.data_type == VDATA_R64 ) {

    if( vec->value.r64->str != NULL ) {
      str = strdup_safe( vec->value.r64->str );
    } else {
      char         width_str[100];
      unsigned int rv = snprintf( width_str, 100, "%f", vec->value.r64->val );
      assert( rv < 100 );
      str = strdup_safe( width_str );
    }

  } else if( vec->suppl.part.data_type == VDATA_R32 ) {

    if( vec->value.r32->str != NULL ) {
      str = strdup_safe( vec->value.r32->str );
    } else {
      char         width_str[30];
      unsigned int rv = snprintf( width_str, 30, "%f", vec->value.r32->val );
      assert( rv < 30 );
      str = strdup_safe( width_str );
    }
 
  } else {

    unsigned int rv;
    char*        tmp;
    unsigned int str_size;
    unsigned int group     = 1;
    char         type_char = 'b';
    char         width_str[20];
    int          vec_size  = ((vec->width - 1) >> 3) + 2;
    int          pos       = 0;

    switch( base ) {
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
        assert( (base == BINARY) || (base == OCTAL)  || (base == HEXIDECIMAL) );
        /*@-unreachable@*/
        break;
        /*@=unreachable@*/
    }

    tmp = (char*)malloc_safe( vec_size );

    switch( vec->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong value = 0;
          int    i;
          for( i=(vec->width - 1); i>=0; i-- ) {
            ulong* entry = vec->value.ul[UL_DIV(i)];
            if( ((entry[VTYPE_INDEX_VAL_VALH] >> UL_MOD(i)) & 0x1) == 1 ) {
              value = ((entry[VTYPE_INDEX_VAL_VALL] >> UL_MOD(i)) & 0x1) + 16;
            } else if( ((entry[VTYPE_INDEX_VAL_VALL] >> UL_MOD(i)) & 0x1) == 1 ) {
              value = (value < 16) ? (((ulong)1 << (UL_MOD(i) % group)) | value) : value;
            }
            assert( pos < vec_size );
            if( (i % group) == 0 ) {
              switch( value ) {
                case 0x0 :  if( (pos > 0) || (i == 0) || show_all ) { tmp[pos] = '0';  pos++; }  break;
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
                  /* Value in vector_to_string exceeds allowed limit */
                  assert( value <= 17 );
                  /*@-unreachable@*/
                  break;
                  /*@=unreachable@*/
              }
              value = 0;
            }
          }
        }
        break;
      default :  assert( 0 );  break;
    }

    tmp[pos] = '\0';

    rv = snprintf( width_str, 20, "%u", vec->width );
    assert( rv < 20 );
    str_size = strlen( width_str ) + 2 + strlen( tmp ) + 1 + vec->suppl.part.is_signed;
    str      = (char*)malloc_safe( str_size );
    if( vec->suppl.part.is_signed == 0 ) {
      rv = snprintf( str, str_size, "%u'%c%s", vec->width, type_char, tmp );
    } else {
      rv = snprintf( str, str_size, "%u's%c%s", vec->width, type_char, tmp );
    }
    assert( rv < str_size );

    free_safe( tmp, vec_size );

  }

  PROFILE_END;

  return( str );

}

/*!
 Converts a string value from the lexer into a vector structure appropriately
 sized.  If the string value size exceeds Covered's maximum bit allowance, return
 a value of NULL to indicate this to the calling function.
*/
void vector_from_string(
  char**   str,    /*!< String version of value */
  bool     quoted, /*!< If TRUE, treat the string as a literal */
  vector** vec,    /*!< Pointer to vector to allocate and populate with string information */
  int*     base    /*!< Base type of string value parsed */
) { PROFILE(VECTOR_FROM_STRING);

  int  bits_per_char;         /* Number of bits represented by a single character in the value string str */
  int  size;                  /* Specifies bit width of vector to create */
  char value[MAX_BIT_WIDTH];  /* String to store string value in */
  char stype[3];              /* Temporary holder for type of string being parsed */
  int  chars_read;            /* Number of characters read by a sscanf() function call */
  bool found_real = FALSE;    /* Specifies that a real value was parsed */

  if( quoted ) {

    size = strlen( *str ) * 8;

    /* If this is the empty (null) string, allocate 8-bits */
    if( size == 0 ) {
      size = 8;
    }

    /* If we have exceeded the maximum number of bits, return a value of NULL */
    if( size > MAX_BIT_WIDTH ) {

      *vec  = NULL;
      *base = 0;

    } else {

      unsigned int pos = 0;
      int          i;

      /* Create vector */
      *vec  = vector_create( size, VTYPE_VAL, VDATA_UL, TRUE );
      *base = QSTRING;

      for( i=(strlen( *str ) - 1); i>=0; i-- ) {
        (*vec)->value.ul[pos>>(UL_DIV_VAL-3)][VTYPE_INDEX_VAL_VALL] |= (ulong)((*str)[i]) << ((pos & (UL_MOD_VAL >> 3)) << 3);
        pos++;
      }

    }

  } else {

    double real;

    if( sscanf( *str, "%d'%[sSdD]%[0-9]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 10;
      *base         = DECIMAL;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%d'%[sSbB]%[01xXzZ_\?]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 1;
      *base         = BINARY;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%d'%[sSoO]%[0-7xXzZ_\?]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 3;
      *base         = OCTAL;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%d'%[sShH]%[0-9a-fA-FxXzZ_\?]%n", &size, stype, value, &chars_read ) == 3 ) {
      bits_per_char = 4;
      *base         = HEXIDECIMAL;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sSdD]%[0-9]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 10;
      *base         = DECIMAL;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sSbB]%[01xXzZ_\?]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 1;
      *base         = BINARY;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sSoO]%[0-7xXzZ_\?]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 3;
      *base         = OCTAL;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "'%[sShH]%[0-9a-fA-FxXzZ_\?]%n", stype, value, &chars_read ) == 2 ) {
      bits_per_char = 4;
      *base         = HEXIDECIMAL;
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%[0-9_]%n", value, &chars_read ) == 1 ) {
      bits_per_char = 10;
      *base         = DECIMAL;
      stype[0]      = 's';       
      stype[1]      = '\0';
      size          = 32;
      *str          = *str + chars_read;
    } else if( sscanf( *str, "%f%n", &real, &chars_read ) == 1 ) {
      found_real                   = TRUE;
      *vec                         = vector_create( 64, VTYPE_VAL, VDATA_R64, TRUE );
      (*vec)->value.r64->val       = real;
      (*vec)->value.r64->str       = strdup_safe( *str );
      (*vec)->suppl.part.is_signed = 1;
      *str                         = *str + chars_read;
    } else if( sscanf( *str, "%g%n", &real, &chars_read ) == 1 ) {
      found_real                   = TRUE;
      *vec                         = vector_create( 64, VTYPE_VAL, VDATA_R64, TRUE );
      (*vec)->value.r64->val       = real;
      (*vec)->value.r64->str       = strdup_safe( *str );
      (*vec)->suppl.part.is_signed = 1;
      *str                         = *str + chars_read;
    } else {
      /* If the specified string is none of the above, return NULL */
      bits_per_char = 0;
    }

    if( !found_real ) {

      /* If we have exceeded the maximum number of bits, return a value of NULL */
      if( (size > MAX_BIT_WIDTH) || (bits_per_char == 0) ) {

        *vec  = NULL;
        *base = 0;

      } else {

        /* Create vector */
        *vec = vector_create( size, VTYPE_VAL, VDATA_UL, TRUE );
        if( *base == DECIMAL ) {
          vector_from_int( *vec, ato32( value ) );
        } else {
          vector_set_static( *vec, value, bits_per_char ); 
        }

        /* Set the signed bit to the appropriate value based on the signed indicator in the vector string */
        if( (stype[0] == 's') || (stype [0] == 'S') ) {
          (*vec)->suppl.part.is_signed = 1;
        } else {
          (*vec)->suppl.part.is_signed = 0;
        }

      }

    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if assigned value differs from the original value; otherwise,
         returns FALSE.

 \throws anonymous Throw

 Iterates through specified value string, setting the specified vector value to
 this value.  Performs a VCD-specific bit-fill if the value size is not the size
 of the vector.  The specified value string is assumed to be in binary format.
*/
bool vector_vcd_assign(
  vector*     vec,    /*!< Pointer to vector to set value to */
  const char* value,  /*!< String version of VCD value */
  int         msb,    /*!< Most significant bit to assign to */
  int         lsb     /*!< Least significant bit to assign to */
) { PROFILE(VECTOR_VCD_ASSIGN);

  bool        retval = FALSE;  /* Return value for this function */
  const char* ptr;             /* Pointer to current character under evaluation */
  int         i;               /* Loop iterator */

  assert( vec != NULL );
  assert( value != NULL );
  assert( (msb < 0) || ((unsigned int)msb <= vec->width) );

  /* Set pointer to LSB */
  ptr = (value + strlen( value )) - 1;
  i   = (lsb > 0) ? lsb : 0;
  msb = (lsb > 0) ? msb : msb;

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong scratchl[UL_DIV(MAX_BIT_WIDTH)];
        ulong scratchh[UL_DIV(MAX_BIT_WIDTH)];
        scratchl[UL_DIV(i)] = 0;
        scratchh[UL_DIV(i)] = 0;
        while( ptr >= value ) {
          unsigned int index  = UL_DIV(i);
          unsigned int offset = UL_MOD(i);
          ulong        bit    = ((ulong)1 << offset);
          if( offset == 0 ) {
            scratchl[index] = 0;
            scratchh[index] = 0;
          }
          scratchl[index] |= ((*ptr == '1') || (*ptr == 'z')) ? bit : 0;
          scratchh[index] |= ((*ptr == 'x') || (*ptr == 'z')) ? bit : 0;
          ptr--;
          i++;
        }
        ptr++;
        /* Bit-fill */
        for( ; i<=msb; i++ ) {
          unsigned int index  = UL_DIV(i);
          unsigned int offset = UL_MOD(i);
          ulong        bit    = ((ulong)1 << offset);
          if( offset == 0 ) {
            scratchl[index] = 0;
            scratchh[index] = 0;
          }
          scratchl[index] |= (*ptr == 'z') ? bit : 0;
          scratchh[index] |= ((*ptr == 'x') || (*ptr == 'z')) ? bit : 0;
        }
        retval = vector_set_coverage_and_assign_ulong( vec, scratchl, scratchh, lsb, msb );
      }
      break;
    case VDATA_R64 :
      if( (sscanf( value, "%f", &(vec->value.r64->val) ) != 1) && (sscanf( value, "%g", &(vec->value.r64->val) ) != 1) ) {
        assert( 0 );
      }
      break;
    case VDATA_R32 :
      if( (sscanf( value, "%f", &(vec->value.r32->val) ) != 1) && (sscanf( value, "%g", &(vec->value.r32->val) ) != 1) ) {
        assert( 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  /* Set the set bit to indicate that this vector has been evaluated */
  vec->suppl.part.set = 1;

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original vector value; otherwise,
         returns FALSE.

 Performs a bitwise AND operation.  Vector sizes will be properly compensated by
 placing zeroes.
*/
bool vector_bitwise_and_op(
  vector* tgt,   /*!< Target vector for operation results to be stored */
  vector* src1,  /*!< Source vector 1 to perform operation on */
  vector* src2   /*!< Source vector 2 to perform operation on */
) { PROFILE(VECTOR_BITWISE_AND_OP);
  
  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      { 
        ulong        scratchl[UL_DIV(MAX_BIT_WIDTH)];
        ulong        scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int src1_size = UL_SIZE(src1->width);
        unsigned int src2_size = UL_SIZE(src2->width);
        unsigned int i;
        for( i=0; i<UL_SIZE(tgt->width); i++ ) {
          ulong* entry1 = src1->value.ul[i];
          ulong* entry2 = src2->value.ul[i];
          ulong  val1_l = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val1_h = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALH] : 0;
          ulong  val2_l = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val2_h = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALH] : 0;
          scratchl[i]   = ~(val1_h | val2_h) & (val1_l & val2_l);
          scratchh[i]   = (val1_h & val2_h) | (val1_h & val2_l) | (val2_h & val1_l);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, scratchl, scratchh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );
 
}

/*!
 \return Returns TRUE if assigned value differs from original vector value; otherwise,
         returns FALSE.

 Performs a bitwise NAND operation.  Vector sizes will be properly compensated by
 placing zeroes.
*/
bool vector_bitwise_nand_op(           
  vector* tgt,   /*!< Target vector for operation results to be stored */
  vector* src1,  /*!< Source vector 1 to perform operation on */
  vector* src2   /*!< Source vector 2 to perform operation on */
) { PROFILE(VECTOR_BITWISE_NAND_OP);   
                                      
  bool retval;  /* Return value for this function */
                                      
  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :                  
      {
        static ulong scratchl[UL_DIV(MAX_BIT_WIDTH)];
        static ulong scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int src1_size = UL_SIZE(src1->width);
        unsigned int src2_size = UL_SIZE(src2->width);
        unsigned int i;
        for( i=0; i<UL_SIZE(tgt->width); i++ ) {
          ulong* entry1 = src1->value.ul[i];
          ulong* entry2 = src2->value.ul[i];
          ulong  val1_l = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val1_h = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALH] : 0;
          ulong  val2_l = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val2_h = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALH] : 0;
          scratchl[i] = ~(val1_h | val2_h) & ~(val1_l & val2_l);
          scratchh[i] = (val1_h & val2_h) | (val1_h & ~val2_l) | (val2_h & ~val1_l);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, scratchl, scratchh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break; 
  }

  PROFILE_END;

  return( retval );
 
}

/*!
 \return Returns TRUE if assigned value differs from original vector value; otherwise,
         returns FALSE.

 Performs a bitwise OR operation.  Vector sizes will be properly compensated by
 placing zeroes.
*/
bool vector_bitwise_or_op(
  vector* tgt,   /*!< Target vector for operation results to be stored */
  vector* src1,  /*!< Source vector 1 to perform operation on */
  vector* src2   /*!< Source vector 2 to perform operation on */
) { PROFILE(VECTOR_BITWISE_OR_OP);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        static ulong scratchl[UL_DIV(MAX_BIT_WIDTH)];
        static ulong scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int src1_size = UL_SIZE(src1->width);
        unsigned int src2_size = UL_SIZE(src2->width);
        unsigned int i;
        for( i=0; i<UL_SIZE(tgt->width); i++ ) {
          ulong* entry1 = src1->value.ul[i];
          ulong* entry2 = src2->value.ul[i];
          ulong  val1_l = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val1_h = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALH] : 0;
          ulong  val2_l = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val2_h = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALH] : 0;
          scratchl[i] = (val1_l & ~val1_h) | (val2_l & ~val2_h);
          scratchh[i] = ~scratchl[i] & (val1_h | val2_h);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, scratchl, scratchh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original vector value; otherwise,
         returns FALSE.

 Performs a bitwise NOR operation.  Vector sizes will be properly compensated by
 placing zeroes.
*/
bool vector_bitwise_nor_op(
  vector* tgt,   /*!< Target vector for operation results to be stored */
  vector* src1,  /*!< Source vector 1 to perform operation on */
  vector* src2   /*!< Source vector 2 to perform operation on */
) { PROFILE(VECTOR_BITWISE_NOR_OP);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        static ulong scratchl[UL_DIV(MAX_BIT_WIDTH)];
        static ulong scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int src1_size = UL_SIZE(src1->width);
        unsigned int src2_size = UL_SIZE(src2->width);
        unsigned int i;
        for( i=0; i<UL_SIZE(tgt->width); i++ ) {
          ulong* entry1 = src1->value.ul[i];
          ulong* entry2 = src2->value.ul[i];
          ulong  val1_l = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val1_h = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALH] : 0;
          ulong  val2_l = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val2_h = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALH] : 0;
          scratchl[i] = ~(val1_h | val2_h) & ~(val1_l | val2_l);
          scratchh[i] =  (val1_h & val2_h) |  (val1_h & val2_l) | (val2_h & val1_l);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, scratchl, scratchh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original vector value; otherwise,
         returns FALSE.

 Performs a bitwise XOR operation.  Vector sizes will be properly compensated by
 placing zeroes.
*/
bool vector_bitwise_xor_op(
  vector* tgt,   /*!< Target vector for operation results to be stored */
  vector* src1,  /*!< Source vector 1 to perform operation on */
  vector* src2   /*!< Source vector 2 to perform operation on */
) { PROFILE(VECTOR_BITWISE_XOR_OP);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        static ulong scratchl[UL_DIV(MAX_BIT_WIDTH)];
        static ulong scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int src1_size = UL_SIZE(src1->width);
        unsigned int src2_size = UL_SIZE(src2->width);
        unsigned int i;
        for( i=0; i<UL_SIZE(tgt->width); i++ ) {
          ulong* entry1 = src1->value.ul[i];
          ulong* entry2 = src2->value.ul[i];
          ulong  val1_l = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val1_h = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALH] : 0;
          ulong  val2_l = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val2_h = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALH] : 0;
          scratchl[i] = (val1_l ^ val2_l) & ~(val1_h | val2_h);
          scratchh[i] = (val1_h | val2_h);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, scratchl, scratchh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original vector value; otherwise,
         returns FALSE.

 Performs a bitwise NXOR operation.  Vector sizes will be properly compensated by
 placing zeroes.
*/
bool vector_bitwise_nxor_op(
  vector* tgt,   /*!< Target vector for operation results to be stored */
  vector* src1,  /*!< Source vector 1 to perform operation on */
  vector* src2   /*!< Source vector 2 to perform operation on */
) { PROFILE(VECTOR_BITWISE_NXOR_OP);
  
  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      { 
        static ulong scratchl[UL_DIV(MAX_BIT_WIDTH)];
        static ulong scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int src1_size = UL_SIZE(src1->width);
        unsigned int src2_size = UL_SIZE(src2->width);
        unsigned int i;
        for( i=0; i<UL_SIZE(tgt->width); i++ ) {
          ulong* entry1 = src1->value.ul[i];
          ulong* entry2 = src2->value.ul[i];
          ulong  val1_l = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val1_h = (i<src1_size) ? entry1[VTYPE_INDEX_VAL_VALH] : 0;
          ulong  val2_l = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALL] : 0;
          ulong  val2_h = (i<src2_size) ? entry2[VTYPE_INDEX_VAL_VALH] : 0;
          scratchl[i] = ~(val1_l ^ val2_l) & ~(val1_h | val2_h);
          scratchh[i] =  (val1_h | val2_h);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, scratchl, scratchh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );
 
}

/*!
 \return Returns TRUE if vectors need to be reversed for comparison purposes; otherwise, returns FALSE.
*/
inline static bool vector_reverse_for_cmp_ulong(
  const vector* left,  /*!< Pointer to left vector that is being compared */
  const vector* right  /*!< Pointer to right vector that is being compared */
) {

  return( left->suppl.part.is_signed && right->suppl.part.is_signed &&
          (((left->value.ul[UL_DIV(left->width-1)][VTYPE_INDEX_VAL_VALL]   >> UL_MOD(left->width  - 1)) & 0x1) !=
           ((right->value.ul[UL_DIV(right->width-1)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(right->width - 1)) & 0x1)) );

}

/*!
 Copies the specified indexed ulong from the given vector and sign extends the value as necessary.
*/
inline static void vector_copy_val_and_sign_extend_ulong(
            const vector* vec,         /*!< Pointer to vector to copy and sign extend */
            unsigned int  index,       /*!< Current ulong index to copy and sign extend */
            bool          msb_is_one,  /*!< Set to TRUE if MSB is a value of one */
  /*@out@*/ ulong*        vall,        /*!< Pointer to the low ulong to copy and sign extend to */
  /*@out@*/ ulong*        valh         /*!< Pointer to the high ulong to copy and sign extend to */
) {

  unsigned int last_index = UL_DIV(vec->width - 1);

  /* If we are at or exceeding the size of the current vector, it is a signed vector and the MSB is one, sign extend */
  if( (index >= last_index) && (vec->suppl.part.is_signed == 1) && msb_is_one ) {
    if( index == last_index ) {
      *vall = vec->value.ul[index][VTYPE_INDEX_VAL_VALL] | UL_LMASK(vec->width);
      *valh = vec->value.ul[index][VTYPE_INDEX_VAL_VALH];
    } else {
      *vall = UL_SET;
      *valh = 0;
    }

  /* Otherwise, if we are exceeding the index, set the vall and valh to 0 */
  } else if( index > last_index ) {
    *vall = 0;
    *valh = 0;

  /* Otherwise, just copy the value */
  } else {
    *vall = vec->value.ul[index][VTYPE_INDEX_VAL_VALL];
    *valh = vec->value.ul[index][VTYPE_INDEX_VAL_VALH];
  }
  
}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a less-than comparison of the left and right expressions.
*/
bool vector_op_lt(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of less than sign */
  const vector* right  /*!< Expression on right of less than sign */
) { PROFILE(VECTOR_OP_LT);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong scratchl;
          ulong scratchh = 0;
          if( (left->suppl.part.data_type == VDATA_UL) && (right->suppl.part.data_type == VDATA_UL) ) {
            unsigned int lsize       = UL_SIZE(left->width);
            unsigned int rsize       = UL_SIZE(right->width);
            int          i           = ((lsize < rsize) ? rsize : lsize);
            unsigned int lmsb        = (left->width - 1);
            unsigned int rmsb        = (right->width - 1);
            bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
            bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
            ulong        lvall;
            ulong        lvalh;
            ulong        rvall;
            ulong        rvalh;
            do {
              i--;
              vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
              vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
            } while( (i > 0) && (lvall == rvall) );
            scratchl = vector_reverse_for_cmp_ulong( left, right ) ? (rvall < lvall) : (lvall < rvall);
          } else {
            scratchl = (vector_to_real64( left ) < vector_to_real64( right )) ? 1 : 0;
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a less-than-or-equal comparison of the left and right expressions.
*/
bool vector_op_le(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of less than sign */
  const vector* right  /*!< Expression on right of less than sign */
) { PROFILE(VECTOR_OP_LE);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong scratchl;
          ulong scratchh = 0;
          if( (left->suppl.part.data_type == VDATA_UL) && (right->suppl.part.data_type == VDATA_UL) ) {
            unsigned int lsize       = UL_SIZE(left->width);
            unsigned int rsize       = UL_SIZE(right->width);
            int          i           = ((lsize < rsize) ? rsize : lsize);
            unsigned int lmsb        = (left->width - 1);
            unsigned int rmsb        = (right->width - 1);
            bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
            bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
            ulong        lvall;
            ulong        lvalh;
            ulong        rvall;
            ulong        rvalh;
            do {
              i--;
              vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
              vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
            } while( (i > 0) && (lvall == rvall) );
            scratchl = vector_reverse_for_cmp_ulong( left, right ) ? (rvall <= lvall) : (lvall <= rvall);
          } else {
            scratchl = (vector_to_real64( left ) <= vector_to_real64( right )) ? 1 : 0;
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a greater-than comparison of the left and right expressions.
*/
bool vector_op_gt(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than sign */
  const vector* right  /*!< Expression on right of greater-than sign */
) { PROFILE(VECTOR_OP_GT);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {
 
    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong scratchl;
          ulong scratchh = 0;
          if( (left->suppl.part.data_type == VDATA_UL) && (right->suppl.part.data_type == VDATA_UL) ) {
            unsigned int lsize       = UL_SIZE(left->width);
            unsigned int rsize       = UL_SIZE(right->width);
            int          i           = ((lsize < rsize) ? rsize : lsize);
            unsigned int lmsb        = (left->width - 1);
            unsigned int rmsb        = (right->width - 1);
            bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
            bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
            ulong        lvall;
            ulong        lvalh;
            ulong        rvall;
            ulong        rvalh;
            do {
              i--;
              vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
              vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
            } while( (i > 0) && (lvall == rvall) );
            scratchl = vector_reverse_for_cmp_ulong( left, right ) ? (rvall > lvall) : (lvall > rvall);
          } else {
            scratchl = (vector_to_real64( left ) > vector_to_real64( right )) ? 1 : 0;
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a greater-than-or-equal comparison of the left and right expressions.
*/
bool vector_op_ge(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than-or-equal sign */
  const vector* right  /*!< Expression on right of greater-than-or-equal sign */
) { PROFILE(VECTOR_OP_GE);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong scratchl;
          ulong scratchh = 0;
          if( (left->suppl.part.data_type == VDATA_UL) && (right->suppl.part.data_type == VDATA_UL) ) {
            unsigned int lsize       = UL_SIZE(left->width);
            unsigned int rsize       = UL_SIZE(right->width);
            int          i           = ((lsize < rsize) ? rsize : lsize);
            unsigned int lmsb        = (left->width - 1);
            unsigned int rmsb        = (right->width - 1);
            bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
            bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
            ulong        lvall;
            ulong        lvalh;
            ulong        rvall;
            ulong        rvalh;
            do {
              i--;
              vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
              vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
            } while( (i > 0) && (lvall == rvall) );
            scratchl = vector_reverse_for_cmp_ulong( left, right ) ? (rvall >= lvall) : (lvall >= rvall);
          } else {
            scratchl = (vector_to_real64( left ) >= vector_to_real64( right )) ? 1 : 0;
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs an equal comparison of the left and right expressions.
*/
bool vector_op_eq(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than-or-equal sign */
  const vector* right  /*!< Expression on right of greater-than-or-equal sign */
) { PROFILE(VECTOR_OP_EQ);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong scratchl;
          ulong scratchh = 0;
          if( (left->suppl.part.data_type == VDATA_UL) && (right->suppl.part.data_type == VDATA_UL) ) {
            unsigned int lsize       = UL_SIZE(left->width);
            unsigned int rsize       = UL_SIZE(right->width);
            int          i           = ((lsize < rsize) ? rsize : lsize);
            unsigned int lmsb        = (left->width - 1);
            unsigned int rmsb        = (right->width - 1);
            bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
            bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
            ulong        lvall;
            ulong        lvalh;
            ulong        rvall;
            ulong        rvalh;
            do {
              i--;
              vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
              vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
            } while( (i > 0) && (lvall == rvall) );
            scratchl = (lvall == rvall);
          } else {
            scratchl = (vector_to_real64( left ) == vector_to_real64( right )) ? 1 : 0;
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the two input vectors are four-state bitwise-equal to each other; otherwise,
         returns FALSE.
*/
bool vector_ceq_ulong(
  const vector* left,   /*!< Pointer to left vector to compare */
  const vector* right   /*!< Pointer to right vector to compare */
) { PROFILE(VECTOR_CEQ_ULONG);

  unsigned int lsize       = UL_SIZE(left->width);
  unsigned int rsize       = UL_SIZE(right->width);
  int          i           = ((lsize < rsize) ? rsize : lsize);
  unsigned int lmsb        = (left->width - 1);
  unsigned int rmsb        = (right->width - 1);
  bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
  bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
  ulong        lvall;
  ulong        lvalh;
  ulong        rvall;
  ulong        rvalh;

  do {
    i--;
    vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
    vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
  } while( (i > 0) && (lvall == rvall) && (lvalh == rvalh) );

  PROFILE_END;

  return( (lvall == rvall) && (lvalh == rvalh) );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a case equal comparison of the left and right expressions.
*/
bool vector_op_ceq(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than-or-equal sign */
  const vector* right  /*!< Expression on right of greater-than-or-equal sign */
) { PROFILE(VECTOR_OP_CEQ);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong scratchl = vector_ceq_ulong( left, right );
        ulong scratchh = 0;
        retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a casex equal comparison of the left and right expressions.
*/
bool vector_op_cxeq(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than-or-equal sign */
  const vector* right  /*!< Expression on right of greater-than-or-equal sign */
) { PROFILE(VECTOR_OP_CXEQ);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        scratchl    = 0;
        ulong        scratchh    = 0;
        unsigned int lsize       = UL_SIZE(left->width);
        unsigned int rsize       = UL_SIZE(right->width);
        int          i           = ((lsize < rsize) ? rsize : lsize);
        unsigned int lmsb        = (left->width - 1);
        unsigned int rmsb        = (right->width - 1);
        bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
        bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
        ulong        mask        = (left->width < right->width) ? UL_HMASK(left->width - 1) : UL_HMASK(right->width - 1);
        ulong        lvall;
        ulong        lvalh;
        ulong        rvall;
        ulong        rvalh;
        do {
          i--;
          vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
          vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
        } while( (i > 0) && (((~(lvall ^ rvall) | lvalh | rvalh) & mask) == mask) );
        scratchl = (((~(lvall ^ rvall) | lvalh | rvalh) & mask) == mask);
        retval   = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a casez equal comparison of the left and right expressions.
*/
bool vector_op_czeq(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than-or-equal sign */
  const vector* right  /*!< Expression on right of greater-than-or-equal sign */
) { PROFILE(VECTOR_OP_CZEQ);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        scratchl    = 0;
        ulong        scratchh    = 0;
        unsigned int lsize       = UL_SIZE(left->width);
        unsigned int rsize       = UL_SIZE(right->width);
        int          i           = ((lsize < rsize) ? rsize : lsize) - 1;
        unsigned int lmsb        = (left->width - 1);
        unsigned int rmsb        = (right->width - 1);
        bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
        bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
        ulong        mask        = (left->width < right->width) ? UL_HMASK(left->width - 1) : UL_HMASK(right->width - 1);
        ulong        lvall;
        ulong        lvalh;
        ulong        rvall;
        ulong        rvalh;
        vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
        vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
        while( (i > 0) && ((((~(lvall ^ rvall) & ~(lvalh ^ rvalh)) | (lvalh & lvall) | (rvalh & rvall)) & mask) == mask) ) {
          mask  = UL_SET;
          i--;
          vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
          vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
        }
        scratchl = ((((~(lvall ^ rvall) & ~(lvalh ^ rvalh)) | (lvalh & lvall) | (rvalh & rvall)) & mask) == mask);
        retval   = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.

 Performs a not-equal comparison of the left and right expressions.
*/
bool vector_op_ne(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than-or-equal sign */
  const vector* right  /*!< Expression on right of greater-than-or-equal sign */
) { PROFILE(VECTOR_OP_NE);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong scratchl;
          ulong scratchh = 0;
          if( (left->suppl.part.data_type == VDATA_UL) && (right->suppl.part.data_type == VDATA_UL) ) {
            unsigned int lsize       = UL_SIZE(left->width);
            unsigned int rsize       = UL_SIZE(right->width);
            int          i           = ((lsize < rsize) ? rsize : lsize);
            unsigned int lmsb        = (left->width - 1);
            unsigned int rmsb        = (right->width - 1);
            bool         lmsb_is_one = (((left->value.ul[UL_DIV(lmsb)][VTYPE_INDEX_VAL_VALL]  >> UL_MOD(lmsb)) & 1) == 1);
            bool         rmsb_is_one = (((right->value.ul[UL_DIV(rmsb)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(rmsb)) & 1) == 1);
            ulong        lvall;
            ulong        lvalh;
            ulong        rvall;
            ulong        rvalh;
            do {
              i--;
              vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
              vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh );
            } while( (i > 0) && (lvall == rvall) );
            scratchl = (lvall != rvall);
          } else {
            scratchl = (vector_to_real64( left ) != vector_to_real64( right )) ? 1 : 0;
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the assigned value differs from the original value; otherwise, returns FALSE.
 
 Performs an case not-equal comparison of the left and right expressions.
*/
bool vector_op_cne(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression on left of greater-than-or-equal sign */
  const vector* right  /*!< Expression on right of greater-than-or-equal sign */
) { PROFILE(VECTOR_OP_CNE);

  bool retval;  /* Return value for this function */
  
  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong scratchl = !vector_ceq_ulong( left, right );
        ulong scratchh = 0;
        retval = vector_set_coverage_and_assign_ulong( tgt, &scratchl, &scratchh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the value of tgt changed as a result of this operation; otherwise, returns FALSE.

 Performs logical-OR operation and calculates coverage information.
*/
bool vector_op_lor(
  vector*       tgt,   /*!< Pointer to vector to store result into */
  const vector* left,  /*!< Pointer to left vector of expression */
  const vector* right  /*!< Pointer to right vector of expression */
) { PROFILE(VECTOR_OP_LOR);

  bool retval;                                 /* Return value for this function */
  bool lunknown = vector_is_unknown( left );   /* Check for left value being unknown */
  bool runknown = vector_is_unknown( right );  /* Check for right value being unknown */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong valh = (lunknown && runknown) ? 1 : 0;
        ulong vall = ((!lunknown && vector_is_not_zero( left )) || (!runknown && vector_is_not_zero( right ))) ? 1 : 0;
        retval     = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the value of tgt changed as a result of this operation; otherwise, returns FALSE.

 Performs logical-AND operation and calculates coverage information.
*/
bool vector_op_land(
  vector*       tgt,   /*!< Pointer to vector to store result into */
  const vector* left,  /*!< Pointer to left vector of expression */
  const vector* right  /*!< Pointer to right vector of expression */
) { PROFILE(VECTOR_OP_LAND);

  bool retval;                                 /* Return value for this function */
  bool lunknown = vector_is_unknown( left );   /* Check for left value being unknown */
  bool runknown = vector_is_unknown( right );  /* Check for right value being unknown */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong valh = (lunknown && runknown) ? 1 : 0;
        ulong vall = ((!lunknown && vector_is_not_zero( left )) && (!runknown && vector_is_not_zero( right ))) ? 1 : 0;
        retval     = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Converts right expression into an integer value and left shifts the left
 expression the specified number of bit locations, zero-filling the LSB.
*/
bool vector_op_lshift(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression value being shifted left */
  const vector* right  /*!< Expression containing number of bit positions to shift */
) { PROFILE(VECTOR_OP_LSHIFT);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else { 

    int shift_val = vector_to_int( right );

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong vall[UL_DIV(MAX_BIT_WIDTH)];
          ulong valh[UL_DIV(MAX_BIT_WIDTH)];
          vector_lshift_ulong( left, vall, valh, shift_val, ((left->width + shift_val) - 1) );
          retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );
    
}
 
/*!
 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Converts right expression into an integer value and right shifts the left
 expression the specified number of bit locations, zero-filling the MSB.
*/
bool vector_op_rshift(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression value being shifted left */
  const vector* right  /*!< Expression containing number of bit positions to shift */
) { PROFILE(VECTOR_OP_RSHIFT);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    int shift_val = vector_to_int( right );

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong vall[UL_DIV(MAX_BIT_WIDTH)];
          ulong valh[UL_DIV(MAX_BIT_WIDTH)];
          vector_rshift_ulong( left, vall, valh, shift_val, (left->width - 1) );
          retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Converts right expression into an integer value and right shifts the left
 expression the specified number of bit locations, sign extending the MSB.
*/
bool vector_op_arshift(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression value being shifted left */
  const vector* right  /*!< Expression containing number of bit positions to shift */
) { PROFILE(VECTOR_OP_ARSHIFT);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( right ) ) {
  
    retval = vector_set_to_x( tgt );
    
  } else {
  
    int shift_val = vector_to_int( right );
    
    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
          ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
          ulong        signl, signh;
          unsigned int msb = left->width - 1;

          vector_rshift_ulong( left, vall, valh, shift_val, msb );
          vector_get_sign_extend_vector_ulong( left, &signl, &signh );
          vector_sign_extend_ulong( vall, valh, signl, signh, (msb - shift_val), tgt->width );
        
          retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs 4-state bitwise addition on left and right expression values.
 Carry bit is discarded (value is wrapped around).
*/
bool vector_op_add(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression value on left side of + sign */
  const vector* right  /*!< Expression value on right side of + sign */
) { PROFILE(VECTOR_OP_ADD);

  bool retval;  /* Return value for this function */

  /* If either the left or right vector is unknown, set the entire value to X */
  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  /* Otherwise, perform the addition operation */
  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
          ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
          ulong        carry = 0;
          bool         lmsb_is_one = (((left->value.ul[UL_DIV(left->width-1)][VTYPE_INDEX_VAL_VALL]   >> UL_MOD(left->width  - 1)) & 1) == 1);
          bool         rmsb_is_one = (((right->value.ul[UL_DIV(right->width-1)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(right->width - 1)) & 1) == 1);
          ulong        lvall, lvalh;
          ulong        rvall, rvalh;
          unsigned int i;
          for( i=0; i<UL_SIZE( tgt->width ); i++ ) {
            vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh ); 
            vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh ); 
            vall[i] = lvall + rvall + carry;
            valh[i] = 0;
            carry   = ((lvall & rvall) | ((lvall | rvall) & ~vall[i])) >> (UL_BITS - 1);
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
        }
        break;
      case VDATA_R64 :
        {
          double result       = vector_to_real64( left ) + vector_to_real64( right );
          retval              = (tgt->value.r64->val != result);
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) + vector_to_real64( right ));
          retval              = (tgt->value.r32->val != result);
          tgt->value.r32->val = result;
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}    

/*!
 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs a twos complement of the src vector and stores the result in the tgt vector.
*/
bool vector_op_negate(
  vector*       tgt,  /*!< Pointer to vector that will be assigned the new value */
  const vector* src   /*!< Pointer to vector that will be negated */
) { PROFILE(VECTOR_OP_NEGATE);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( src ) ) {
   
    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          if( src->width <= UL_BITS ) {
            ulong vall = ~src->value.ul[0][VTYPE_INDEX_EXP_VALL] + 1;
            ulong valh = 0;

            retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, (tgt->width - 1) );
          } else {
            ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
            ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
            unsigned int i, j;
            unsigned int size  = UL_SIZE( src->width );
            ulong        carry = 1;
            ulong        val;

            for( i=0; i<(size - 1); i++ ) {
              val     = ~src->value.ul[i][VTYPE_INDEX_EXP_VALL];
              vall[i] = 0;
              valh[i] = 0;
              for( j=0; j<UL_BITS; j++ ) {
                ulong bit = ((val >> j) & 0x1) + carry;
                carry     = bit >> 1;
                vall[i]  |= (bit & 0x1) << j;
              }
            }
            val     = ~src->value.ul[i][VTYPE_INDEX_EXP_VALL];
            vall[i] = 0;
            valh[i] = 0;
            for( j=0; j<(tgt->width - (i << UL_DIV_VAL)); j++ ) {
              ulong bit = ((val >> j) & 0x1) + carry;
              carry     = bit >> 1;
              vall[i]  |= (bit & 0x1) << j;
            }

            retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
          }
        }
        break;
      case VDATA_R64 :
        {
          double result       = (0.0 - vector_to_real64( src ));
          retval              = (tgt->value.r64->val != result);
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(0.0 - vector_to_real64( src ));
          retval              = (tgt->value.r32->val != result);
          tgt->value.r32->val = result;
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs 4-state bitwise subtraction by performing bitwise inversion
 of right expression value, adding one to the result and adding this
 result to the left expression value.
*/
bool vector_op_subtract(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression value on left side of - sign */
  const vector* right  /*!< Expression value on right side of - sign */
) { PROFILE(VECTOR_OP_SUBTRACT);

  bool retval;  /* Return value for this function */

  /* If either the left or right vector is unknown, set the entire value to X */
  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  /* Otherwise, perform the subtraction operation */
  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
          ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
          ulong        carry = 1;
          bool         lmsb_is_one = (((left->value.ul[UL_DIV(left->width-1)][VTYPE_INDEX_VAL_VALL]   >> UL_MOD(left->width  - 1)) & 1) == 1);
          bool         rmsb_is_one = (((right->value.ul[UL_DIV(right->width-1)][VTYPE_INDEX_VAL_VALL] >> UL_MOD(right->width - 1)) & 1) == 1);
          ulong        lvall, lvalh;
          ulong        rvall, rvalh;
          unsigned int i;
          for( i=0; i<UL_SIZE( tgt->width ); i++ ) {
            vector_copy_val_and_sign_extend_ulong( left,  i, lmsb_is_one, &lvall, &lvalh );
            vector_copy_val_and_sign_extend_ulong( right, i, rmsb_is_one, &rvall, &rvalh ); 
            rvall   = ~rvall;
            vall[i] = lvall + rvall + carry;
            valh[i] = 0;
            carry   = ((lvall & rvall) | ((lvall | rvall) & ~vall[i])) >> (UL_BITS - 1);
          }
          retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
        }
        break;
      case VDATA_R64 :
        {
          double result       = vector_to_real64( left ) - vector_to_real64( right );
          retval              = (tgt->value.r64->val != result);
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) - vector_to_real64( right ));
          retval              = (tgt->value.r32->val != result);
          tgt->value.r32->val = result;
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original value; otherwise, returns FALSE.

 Performs 4-state conversion multiplication.  If both values
 are known (non-X, non-Z), vectors are converted to integers, multiplication
 occurs and values are converted back into vectors.  If one of the values
 is equal to zero, the value is 0.  If one of the values is equal to one,
 the value is that of the other vector.
*/
bool vector_op_multiply(
  vector*       tgt,   /*!< Target vector for storage of results */
  const vector* left,  /*!< Expression value on left side of * sign */
  const vector* right  /*!< Expression value on right side of * sign */
) { PROFILE(VECTOR_OP_MULTIPLY);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong vall = left->value.ul[0][VTYPE_INDEX_VAL_VALL] * right->value.ul[0][VTYPE_INDEX_VAL_VALL];
          ulong valh = 0;
          retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, (tgt->width - 1) );
        }
        break;
      case VDATA_R64 :
        {
          double result       = vector_to_real64( left ) * vector_to_real64( right );
          retval              = (tgt->value.r64->val != result);
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) * vector_to_real64( right ));
          retval              = (tgt->value.r32->val != result);
          tgt->value.r32->val = result;
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if value changes; otherwise, returns FALSE.

 Performs vector divide operation.
*/
bool vector_op_divide(
  vector*       tgt,   /*!< Pointer to vector that will store divide result */
  const vector* left,  /*!< Pointer to left vector */
  const vector* right  /*!< Pointer to right vector */
) { PROFILE(VECTOR_OP_DIVIDE);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong vall;
          ulong valh = 0;
          ulong rval = right->value.ul[0][VTYPE_INDEX_EXP_VALL];
          if( rval == 0 ) {
            retval = vector_set_to_x( tgt );
          } else {
            vall = left->value.ul[0][VTYPE_INDEX_EXP_VALL] / rval;
            retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, (UL_BITS - 1) );
          }
        }
        break;
      case VDATA_R64 :
        {
          double result       = vector_to_real64( left ) / vector_to_real64( right );
          retval              = (tgt->value.r64->val != result);
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) / vector_to_real64( right ));
          retval              = (tgt->value.r32->val != result);
          tgt->value.r32->val = result;
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if value changes; otherwise, returns FALSE.

 Performs vector modulus operation.
*/
bool vector_op_modulus(
  vector*       tgt,   /*!< Pointer to vector that will store divide result */
  const vector* left,  /*!< Pointer to left vector */
  const vector* right  /*!< Pointer to right vector */
) { PROFILE(VECTOR_OP_MODULUS);

  bool retval;  /* Return value for this function */

  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( tgt->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong vall;
          ulong valh = 0;
          ulong rval = right->value.ul[0][VTYPE_INDEX_EXP_VALL];
          if( rval == 0 ) {
            retval = vector_set_to_x( tgt );
          } else {
            vall = left->value.ul[0][VTYPE_INDEX_EXP_VALL] % rval;
            retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, (UL_BITS - 1) );
          }
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE (increments will always cause a value change)

 Performs an increment operation on the specified vector.
*/
bool vector_op_inc(
  vector* tgt,  /*!< Target vector to assign data to */
  vecblk* tvb   /*!< Pointer to vector block for temporary vectors */
) { PROFILE(VECTOR_OP_INC);

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        vector* tmp1 = &(tvb->vec[tvb->index++]);
        vector* tmp2 = &(tvb->vec[tvb->index++]);
        vector_copy( tgt, tmp1 );
        tmp2->value.ul[0][VTYPE_INDEX_VAL_VALL] = 1;
        (void)vector_op_add( tgt, tmp1, tmp2 );
      }
      break;
    case VDATA_R64 :
      tgt->value.r64->val += 1.0;
      break;
    case VDATA_R32 :
      tgt->value.r32->val += 1.0;
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE (decrements will always cause a value change)

 Performs an decrement operation on the specified vector.
*/
bool vector_op_dec(
  vector* tgt,  /*!< Target vector to assign data to */
  vecblk* tvb   /*!< Pointer to vector block for temporary vectors */
) { PROFILE(VECTOR_OP_DEC);

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        vector* tmp1 = &(tvb->vec[tvb->index++]);
        vector* tmp2 = &(tvb->vec[tvb->index++]);
        vector_copy( tgt, tmp1 );
        tmp2->value.ul[0][VTYPE_INDEX_VAL_VALL] = 1;
        (void)vector_op_subtract( tgt, tmp1, tmp2 );
      }
    case VDATA_R64 :
      tgt->value.r64->val -= 1.0;
      break;
    case VDATA_R32 :
      tgt->value.r32->val -= 1.0;
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if assigned value differs from orignal; otherwise, returns FALSE.

 Performs a bitwise inversion on the specified vector.
*/
bool vector_unary_inv(
  vector*       tgt,  /*!< Target vector for operation results to be stored */
  const vector* src   /*!< Source vector to perform operation on */
) { PROFILE(VECTOR_UNARY_INV);

  bool retval;  /* Return value for this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
        ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
        ulong        mask = UL_HMASK(src->width - 1);
        ulong        tvalh;
        unsigned int i;
        unsigned int size = UL_SIZE( src->width );

        for( i=0; i<(size-1); i++ ) {
          tvalh   = src->value.ul[i][VTYPE_INDEX_EXP_VALH];
          vall[i] = ~tvalh & ~src->value.ul[i][VTYPE_INDEX_EXP_VALL];
          valh[i] = tvalh;
        }
        tvalh   = src->value.ul[i][VTYPE_INDEX_EXP_VALH];
        vall[i] = ~tvalh & ~src->value.ul[i][VTYPE_INDEX_EXP_VALL] & mask;
        valh[i] = tvalh & mask;

        retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary AND operation on specified vector value.
*/
bool vector_unary_and(
  vector*       tgt,  /*!< Target vector for operation result storage */
  const vector* src   /*!< Source vector to be operated on */
) { PROFILE(VECTOR_UNARY_AND);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i;
        unsigned int ssize = UL_SIZE( src->width );
        ulong        valh  = 0;
        ulong        vall  = 1;
        ulong        lmask = UL_HMASK(src->width - 1);
        for( i=0; i<(ssize-1); i++ ) {
          valh |= (src->value.ul[i][VTYPE_INDEX_VAL_VALH] != 0) ? 1 : 0;
          vall &= ~valh & ((src->value.ul[i][VTYPE_INDEX_VAL_VALL] == UL_SET) ? 1 : 0);
        }
        valh |= (src->value.ul[i][VTYPE_INDEX_VAL_VALH] != 0) ? 1 : 0;
        vall &= ~valh & ((src->value.ul[i][VTYPE_INDEX_VAL_VALL] == lmask) ? 1 : 0);
        retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary NAND operation on specified vector value.
*/
bool vector_unary_nand(
  vector*       tgt,  /*!< Target vector for operation result storage */
  const vector* src   /*!< Source vector to be operated on */
) { PROFILE(VECTOR_UNARY_NAND);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i;
        unsigned int ssize = UL_SIZE( src->width );
        ulong        valh  = 0;
        ulong        vall  = 0;
        ulong        lmask = UL_HMASK(src->width - 1);
        for( i=0; i<(ssize-1); i++ ) {
          valh |= (src->value.ul[i][VTYPE_INDEX_VAL_VALH] != 0) ? 1 : 0;
          vall |= ~valh & ((src->value.ul[i][VTYPE_INDEX_VAL_VALL] == UL_SET) ? 0 : 1);
        }
        valh |= (src->value.ul[i][VTYPE_INDEX_VAL_VALH] != 0) ? 1 : 0;
        vall |= ~valh & ((src->value.ul[i][VTYPE_INDEX_VAL_VALL] == lmask) ? 0 : 1);
        retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  } 

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary OR operation on specified vector value.
*/
bool vector_unary_or(
  vector*       tgt,  /*!< Target vector for operation result storage */
  const vector* src   /*!< Source vector to be operated on */
) { PROFILE(VECTOR_UNARY_OR);

  bool retval;  /* Return value for this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        vall;
        ulong        valh;
        unsigned int i    = 0;
        unsigned int size = UL_SIZE( src->width );
        ulong        x    = 0;
        while( (i < size) && ((~src->value.ul[i][VTYPE_INDEX_VAL_VALH] & src->value.ul[i][VTYPE_INDEX_VAL_VALL]) == 0) ) {
          x |= src->value.ul[i][VTYPE_INDEX_VAL_VALH];
          i++;
        }
        if( i < size ) {
          vall = 1;
          valh = 0;
        } else {
          vall = 0;
          valh = (x != 0);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary NOR operation on specified vector value.
*/
bool vector_unary_nor(
  vector*       tgt,  /*!< Target vector for operation result storage */
  const vector* src   /*!< Source vector to be operated on */
) { PROFILE(VECTOR_UNARY_NOR);

  bool retval;  /* Return value for this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        vall;
        ulong        valh;
        unsigned int i    = 0;
        unsigned int size = UL_SIZE( src->width );
        ulong        x    = 0;
        while( (i < size) && ((~src->value.ul[i][VTYPE_INDEX_VAL_VALH] & src->value.ul[i][VTYPE_INDEX_VAL_VALL]) == 0) ) {
          x |= src->value.ul[i][VTYPE_INDEX_VAL_VALH];
          i++;
        }
        if( i < size ) {
          vall = 0;
          valh = 0;
        } else {
          vall = (x == 0);
          valh = (x != 0);
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary XOR operation on specified vector value.
*/
bool vector_unary_xor(
  vector*       tgt,  /*!< Target vector for operation result storage */
  const vector* src   /*!< Source vector to be operated on */
) { PROFILE(VECTOR_UNARY_XOR);

  bool retval;  /* Return value for this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        vall = 0;
        ulong        valh = 0;
        unsigned int i    = 0;
        unsigned int size = UL_SIZE( src->width );
        do {
          if( src->value.ul[i][VTYPE_INDEX_VAL_VALH] != 0 ) {
            vall = 0;
            valh = 1;
          } else {
            unsigned int j;
            ulong        tval = src->value.ul[i][VTYPE_INDEX_VAL_VALL];
            for( j=1; j<UL_BITS; j<<=1 ) {
              tval = tval ^ (tval >> j);
            }
            vall = (vall ^ tval) & 0x1;
          }
          i++;
        } while( (i < size) && (valh == 0) );
        retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary NXOR operation on specified vector value.
*/
bool vector_unary_nxor(
  vector*       tgt,  /*!< Target vector for operation result storage */
  const vector* src   /*!< Source vector to be operated on */
) { PROFILE(VECTOR_UNARY_NXOR);

  bool retval;  /* Return value for this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        vall = 1;
        ulong        valh = 0;
        unsigned int i    = 0;
        unsigned int size = UL_SIZE( src->width );
        do {
          if( src->value.ul[i][VTYPE_INDEX_VAL_VALH] != 0 ) {
            vall = 0;
            valh = 1;
          } else {
            unsigned int j;
            ulong        tval = src->value.ul[i][VTYPE_INDEX_VAL_VALL];
            for( j=1; j<UL_BITS; j<<=1 ) {
              tval = tval ^ (tval >> j);
            }
            vall = (vall ^ tval) & 0x1;
          }
          i++;
        } while( (i < size) && (valh == 0) );
        retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if assigned value differs from original; otherwise, returns FALSE.

 Performs unary logical NOT operation on specified vector value.
*/
bool vector_unary_not(
  vector*       tgt,  /*!< Target vector for operation result storage */
  const vector* src   /*!< Source vector to be operated on */
) { PROFILE(VECTOR_UNARY_NOT);

  bool retval;  /* Return value of this function */

  switch( src->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        vall;
        ulong        valh;
        unsigned int size = UL_SIZE( src->width );
        unsigned int i    = 0;
        while( (i < size) && (src->value.ul[i][VTYPE_INDEX_VAL_VALH] == 0) && (src->value.ul[i][VTYPE_INDEX_VAL_VALL] == 0) ) i++;
        if( i < size ) {
          vall = 0;
          valh = (src->value.ul[i][VTYPE_INDEX_VAL_VALH] != 0);
        } else {
          vall = 1;
          valh = 0;
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, 0 );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the new value differs from the old value; otherwise, returns FALSE.

 Performs expansion operation.
*/
bool vector_op_expand(
  vector*       tgt,   /*!< Pointer to vector to store results in */
  const vector* left,  /*!< Pointer to vector containing expansion multiplier value */
  const vector* right  /*!< Pointer to vector that will be multiplied */
) { PROFILE(VECTOR_OP_EXPAND);

  bool retval;  /* Return value for this function */

  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
        ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int i, j;
        unsigned int rwidth     = right->width;
        unsigned int multiplier = vector_to_int( left );
        unsigned int pos        = 0;
        for( i=0; i<multiplier; i++ ) {
          for( j=0; j<rwidth; j++ ) {
            ulong*       rval     = right->value.ul[UL_DIV(j)];
            unsigned int my_index = UL_DIV(pos);
            unsigned int offset   = UL_MOD(pos);
            if( offset == 0 ) {
              vall[my_index] = 0;
              valh[my_index] = 0;
            }
            vall[my_index] |= ((rval[VTYPE_INDEX_VAL_VALL] >> UL_MOD(j)) & 0x1) << offset;
            valh[my_index] |= ((rval[VTYPE_INDEX_VAL_VALH] >> UL_MOD(j)) & 0x1) << offset;
            pos++;
          }
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, (tgt->width - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the new value differs from the old value; otherwise, returns FALSE.

 Performs list operation.
*/
bool vector_op_list(
  vector*       tgt,   /*!< Pointer to vector to store results in */
  const vector* left,  /*!< Pointer to vector containing expansion multiplier value */
  const vector* right  /*!< Pointer to vector that will be multiplied */
) { PROFILE(VECTOR_OP_LIST);
        
  bool retval;  /* Return value for this function */
          
  switch( tgt->suppl.part.data_type ) {
    case VDATA_UL :
      {       
        ulong        vall[UL_DIV(MAX_BIT_WIDTH)];
        ulong        valh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int i;
        unsigned int pos    = right->width;
        unsigned int lwidth = left->width;
        unsigned int rsize  = UL_SIZE( pos );

        /* Load right vector directly */
        for( i=0; i<rsize; i++ ) {
          ulong* rval = right->value.ul[i];
          vall[i] = rval[VTYPE_INDEX_VAL_VALL];
          valh[i] = rval[VTYPE_INDEX_VAL_VALH];
        }

        /* Load left vector a bit at at time */
        for( i=0; i<lwidth; i++ ) {
          ulong*       lval     = left->value.ul[UL_DIV(i)];
          unsigned int my_index = UL_DIV(pos);
          unsigned int offset   = UL_MOD(pos);
          if( offset == 0 ) {
            vall[my_index] = 0;
            valh[my_index] = 0;
          }
          vall[my_index] |= ((lval[VTYPE_INDEX_EXP_VALL] >> UL_MOD(i)) & 0x1) << offset;
          valh[my_index] |= ((lval[VTYPE_INDEX_EXP_VALH] >> UL_MOD(i)) & 0x1) << offset;
          pos++;
        }
        retval = vector_set_coverage_and_assign_ulong( tgt, vall, valh, 0, ((left->width + right->width) - 1) );
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );
  
}

/*!
 Deallocates the value structure for the given vector.
*/
void vector_dealloc_value(
  vector* vec  /*!< Pointer to vector to deallocate value for */
) { PROFILE(VECTOR_DEALLOC_VALUE);

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        unsigned int i;
        unsigned int size = UL_SIZE( vec->width );

        for( i=0; i<size; i++ ) {
          free_safe( vec->value.ul[i], (sizeof( ulong ) * vector_type_sizes[vec->suppl.part.type]) );
        }
        free_safe( vec->value.ul, (sizeof( ulong* ) * size) );
        vec->value.ul = NULL;
      }
      break;
    case VDATA_R64 :
      free_safe( vec->value.r64->str, (strlen( vec->value.r64->str ) + 1) );
      free_safe( vec->value.r64, sizeof( rv64 ) );
      break;
    case VDATA_R32 :
      free_safe( vec->value.r32->str, (strlen( vec->value.r32->str ) + 1) );
      free_safe( vec->value.r32, sizeof( rv32 ) );
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

}

/*!
 Deallocates all heap memory that was initially allocated with the malloc
 routine.
*/
void vector_dealloc(
  vector* vec  /*!< Pointer to vector to deallocate memory from */
) { PROFILE(VECTOR_DEALLOC);

  if( vec != NULL ) {

    /* Deallocate all vector values */
    if( (vec->value.ul != NULL) && (vec->suppl.part.owns_data == 1) ) {
      vector_dealloc_value( vec );
    }

    /* Deallocate vector itself */
    free_safe( vec, sizeof( vector ) );

  }

  PROFILE_END;

}

/*
 $Log$
 Revision 1.166  2008/10/17 23:20:51  phase1geo
 Continuing to add support support for real values.  Making some good progress here
 (real delays should be working now).  Updated regressions per recent changes.
 Checkpointing.

 Revision 1.165  2008/10/17 07:26:49  phase1geo
 Updating regressions per recent changes and doing more work to fixing real
 value bugs (still not working yet).  Checkpointing.

 Revision 1.164  2008/10/16 23:11:50  phase1geo
 More work on support for real numbers.  I believe that all of the code now
 exists in vector.c to support them.  Still need to do work in expr.c.  Added
 two new tests for real numbers to begin verifying their support (they both do
 not currently pass, however).  Checkpointing.

 Revision 1.163  2008/10/16 05:16:06  phase1geo
 More work on real number support.  Still a work in progress.  Checkpointing.

 Revision 1.162  2008/10/15 13:28:37  phase1geo
 Beginning to add support for real numbers.  Things are broken in regards
 to real numbers at the moment.  Checkpointing.

 Revision 1.161  2008/10/07 22:31:42  phase1geo
 Cleaning up splint warnings.  Cleaning up development documentation.

 Revision 1.160  2008/10/03 03:13:49  phase1geo
 Fixing problem with vector_from_int and vector_from_uint64 changes.  Full regression
 passes again.

 Revision 1.159  2008/10/02 22:54:23  phase1geo
 Attempting to fix vector_from_int and vector_from_uint64 warnings.

 Revision 1.158  2008/10/02 21:39:25  phase1geo
 Fixing issues with $random system task call.  Added more diagnostics to verify
 its functionality.

 Revision 1.157  2008/09/30 23:13:32  phase1geo
 Checkpointing (TOT is broke at this point).

 Revision 1.156  2008/09/26 22:38:59  phase1geo
 Added feature request 2129623.  Updated regressions for this change (except for
 VCS regression at this point).

 Revision 1.155  2008/09/15 05:00:19  phase1geo
 Documentation updates.

 Revision 1.154  2008/09/15 03:43:49  phase1geo
 Cleaning up splint warnings.

 Revision 1.153  2008/09/11 20:19:50  phase1geo
 Fixing !== operator (bug 2106313).  Adding more diagnostics to regression to
 verify codegen code.

 Revision 1.152  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.142.2.3  2008/07/10 22:43:55  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.150  2008/06/27 14:02:04  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.149  2008/06/25 12:38:32  phase1geo
 Fixing one issue with an array overflow in the stype array (array should have
 been 3 characters long when it was only two).  This is an attempt to fix bug
 2001894 though I am unable to confirm it at this time.

 Revision 1.148  2008/06/23 15:55:25  phase1geo
 Fixing other vector issues related to bug 2000732.

 Revision 1.147  2008/06/23 15:48:33  phase1geo
 Fixing bug 2000732.

 Revision 1.146  2008/06/22 22:02:02  phase1geo
 Fixing regression issues.

 Revision 1.145  2008/06/20 15:48:23  phase1geo
 Fixing bug 1994896.  Also removing some commented out printf lines that are
 no longer necessary.

 Revision 1.144  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.143  2008/06/16 23:10:43  phase1geo
 Fixing cdd_diff script for error found while running regressions.  Also integrating
 source code fixes from the covered-20080603-branch2 branch.  Full regression passes.

 Revision 1.142.2.2  2008/06/16 04:35:21  phase1geo
 Fixing reading issue with vectors that seems to pop up when running on a 64-bit
 machine with the -m32 option.

 Revision 1.142.2.1  2008/06/10 05:06:30  phase1geo
 Fixed bug 1989398.

 Revision 1.142  2008/06/03 04:43:24  phase1geo
 Fixing bug 1982530.  Updating regression tests.

 Revision 1.141  2008/05/30 23:00:48  phase1geo
 Fixing Doxygen comments to eliminate Doxygen warning messages.

 Revision 1.140  2008/05/30 13:56:22  phase1geo
 Tweaking the add/subtract functions to remove some unnecessary logic in
 the calculation of the carry bit.  Full regressions still pass.

 Revision 1.139  2008/05/30 05:38:33  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.138.2.92  2008/05/29 23:04:51  phase1geo
 Last set of submissions to get full regression passing.  Fixed a few more
 bugs in vector.c and reentrant.c.

 Revision 1.138.2.91  2008/05/29 18:47:03  phase1geo
 Fixing various bugs found in 64-bit mode in vector.c file.  IV and Cver
 regressions pass on 64-bit architecturs.  Checkpointing.

 Revision 1.138.2.90  2008/05/29 06:54:12  phase1geo
 Fixing compatibility problems in 64-bit mode.  More work to go.  Checkpointing.

 Revision 1.138.2.89  2008/05/29 06:46:25  phase1geo
 Finishing last submission.

 Revision 1.138.2.88  2008/05/28 22:12:31  phase1geo
 Adding further support for 32-/64-bit support.  Checkpointing.

 Revision 1.138.2.87  2008/05/28 05:57:12  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.138.2.86  2008/05/27 23:05:08  phase1geo
 Fixing sign-extension issue with signal assignments.  Added diagnostic to
 verify this functionality.  Full regression passes.

 Revision 1.138.2.85  2008/05/27 05:52:51  phase1geo
 Starting to add fix for sign extension.  Not finished at this point.

 Revision 1.138.2.84  2008/05/26 05:42:09  phase1geo
 Adding new error merge diagnostics to regression suite to verify missing vector_db_merge
 error cases.  Full regression passes.

 Revision 1.138.2.83  2008/05/25 05:15:40  phase1geo
 Updating regressions.

 Revision 1.138.2.82  2008/05/25 04:27:33  phase1geo
 Adding div1 and mod1 diagnostics to regression suite.

 Revision 1.138.2.81  2008/05/24 21:20:56  phase1geo
 Fixing bugs with comparison functions when values contain Xs.  Adding more diagnostics
 to regression suite to cover coverage holes.

 Revision 1.138.2.80  2008/05/24 05:36:22  phase1geo
 Fixing bitwise coverage functionality and updating regression files.  Added
 new bitwise1 and err5.1 diagnostics to regression suite.  Removing output
 for uncovered exceptions in command-line parsers.

 Revision 1.138.2.79  2008/05/23 23:04:56  phase1geo
 Adding err5 diagnostic to regression suite.  Fixing memory deallocation bug
 found with err5.  Full regression passes.

 Revision 1.138.2.78  2008/05/23 14:50:23  phase1geo
 Optimizing vector_op_add and vector_op_subtract algorithms.  Also fixing issue with
 vector set bit.  Updating regressions per this change.

 Revision 1.138.2.77  2008/05/16 16:55:15  phase1geo
 Fixing issue in vector_op_add.

 Revision 1.138.2.76  2008/05/16 14:00:36  phase1geo
 Fixing width extension with sign-extend for comparison operators.  Updating
 regression files.  Checkpointing.

 Revision 1.138.2.75  2008/05/15 21:58:12  phase1geo
 Updating regression files per changes for increment and decrement operators.
 Checkpointing.

 Revision 1.138.2.74  2008/05/15 07:02:04  phase1geo
 Another attempt to fix static_afunc1 diagnostic failure.  Checkpointing.

 Revision 1.138.2.73  2008/05/14 02:28:15  phase1geo
 Another attempt to fix toggle issue.  IV and CVer regressions pass again.  Still need to
 complete VCS regression.  Checkpointing.

 Revision 1.138.2.72  2008/05/13 21:56:20  phase1geo
 Checkpointing changes.

 Revision 1.138.2.71  2008/05/13 06:42:25  phase1geo
 Finishing up initial pass of part-select code modifications.  Still getting an
 error in regression.  Checkpointing.

 Revision 1.138.2.70  2008/05/12 23:12:04  phase1geo
 Ripping apart part selection code and reworking it.  Things compile but are
 functionally quite broken at this point.  Checkpointing.

 Revision 1.138.2.69  2008/05/09 22:07:50  phase1geo
 Updates for VCS regressions.  Fixing some issues found in that regression
 suite.  Checkpointing.

 Revision 1.138.2.68  2008/05/09 16:47:51  phase1geo
 Recoding vector_rshift_uint32 function and updating the LAST of the IV
 failures!  Moving onto VCS regression diagnostics...  Checkpointing.

 Revision 1.138.2.67  2008/05/09 04:46:10  phase1geo
 Fixing signedness handling for comparison operations.  Updating regression files.
 Checkpointing.

 Revision 1.138.2.66  2008/05/07 23:09:11  phase1geo
 Fixing vector_mem_wr_count function and calling code.  Updating regression
 files accordingly.  Checkpointing.

 Revision 1.138.2.65  2008/05/07 21:09:10  phase1geo
 Added functionality to allow to_string to output full vector bits (even
 non-significant bits) for purposes of reporting for FSMs (matches original
 behavior).

 Revision 1.138.2.64  2008/05/07 20:50:21  phase1geo
 Fixing bit-fill bug in vector_part_select_push function.  Updated regression
 files.  Checkpointing.

 Revision 1.138.2.63  2008/05/07 19:27:15  phase1geo
 Fixing vector_part_select_push functionality to only set coverage and assign
 information within the target range (not the entire target vector).  Updating
 regression files and checkpointing.

 Revision 1.138.2.62  2008/05/07 18:09:50  phase1geo
 Fixing issue with VCD assign functionality.  Updating regression files
 per this change.  Checkpointing.

 Revision 1.138.2.61  2008/05/07 05:22:51  phase1geo
 Fixing reporting bug with line coverage for continuous assignments.  Updating
 regression files and checkpointing.

 Revision 1.138.2.60  2008/05/07 04:08:51  phase1geo
 Fixing bug with logical AND functionality.  Updating regression files.
 Checkpointing.

 Revision 1.138.2.59  2008/05/07 03:48:21  phase1geo
 Fixing bug with bitwise OR function.  Updating regression files.  Checkpointing.

 Revision 1.138.2.58  2008/05/06 20:51:58  phase1geo
 Fixing bugs with casex and casez.  Checkpointing.

 Revision 1.138.2.57  2008/05/06 06:33:13  phase1geo
 Updating regression files per latest submission of vector.c.  Checkpointing.

 Revision 1.138.2.56  2008/05/06 06:17:01  phase1geo
 Fixing memory errors in vector_set_value_uint32.  Checkpointing.

 Revision 1.138.2.55  2008/05/06 05:45:44  phase1geo
 Attempting to add bit-fill to vector_set_value_uint32 function but it seems
 that I have possibly created a memory overflow.  Checkpointing.

 Revision 1.138.2.54  2008/05/06 04:51:38  phase1geo
 Fixing issue with toggle coverage.  Updating regression files.  Checkpointing.

 Revision 1.138.2.53  2008/05/05 23:49:52  phase1geo
 Fixing case equality function and updating regressions accordingly.  Checkpointing.

 Revision 1.138.2.52  2008/05/05 19:49:59  phase1geo
 Updating regressions, fixing bugs and added new diagnostics.  Checkpointing.

 Revision 1.138.2.51  2008/05/05 12:57:04  phase1geo
 Checkpointing some changes for bit-filling in vector_part_select_push (this is
 not complete at this time).

 Revision 1.138.2.50  2008/05/04 22:05:29  phase1geo
 Adding bit-fill in vector_set_static and changing name of old bit-fill functions
 in vector.c to sign_extend to reflect their true nature.  Added new diagnostics
 to regression suite to verify single-bit select bit-fill (these tests do not work
 at this point).  Checkpointing.

 Revision 1.138.2.49  2008/05/04 05:48:40  phase1geo
 Attempting to fix expression_assign.  Updated regression files.

 Revision 1.138.2.48  2008/05/04 04:22:11  phase1geo
 Fixing vector_to_string bug for quoted strings and updating regression files.
 Checkpointing.

 Revision 1.138.2.47  2008/05/03 23:58:22  phase1geo
 Removing skipped vector functions and updating regression suite files
 accordingly.  Checkpointing.

 Revision 1.138.2.46  2008/05/03 20:10:38  phase1geo
 Fixing some bugs, completing initial pass of vector_op_multiply and updating
 regression files accordingly.  Checkpointing.

 Revision 1.138.2.45  2008/05/03 04:06:54  phase1geo
 Fixing some arc bugs and updating regressions accordingly.  Checkpointing.

 Revision 1.138.2.44  2008/05/02 22:06:13  phase1geo
 Updating arc code for new data structure.  This code is completely untested
 but does compile and has been completely rewritten.  Checkpointing.

 Revision 1.138.2.43  2008/05/02 05:02:20  phase1geo
 Completed initial pass of bit-fill code in vector_part_select_push function.
 Updating regression files.  Checkpointing.

 Revision 1.138.2.42  2008/05/01 23:10:20  phase1geo
 Fix endianness issues and attempting to fix assignment bit-fill functionality.
 Checkpointing.

 Revision 1.138.2.41  2008/05/01 19:40:18  phase1geo
 Fixing bug in unary_or operation.

 Revision 1.138.2.40  2008/05/01 18:18:49  phase1geo
 Implemented initial version of unary (reduction) inclusive OR function.
 Updated regressions.

 Revision 1.138.2.39  2008/05/01 17:51:17  phase1geo
 Fixing bit_fill bug and a few other vector/expression bugs and updating regressions.

 Revision 1.138.2.38  2008/05/01 06:09:34  phase1geo
 Adding arshift operation functionality and reworking the bit_fill code.
 Updating regression files accordingly.  Checkpointing.

 Revision 1.138.2.37  2008/05/01 04:08:30  phase1geo
 Fixing bugs with assignment propagation.

 Revision 1.138.2.36  2008/04/30 23:12:31  phase1geo
 Fixing simulation issues.

 Revision 1.138.2.35  2008/04/30 21:59:56  phase1geo
 Finishing initial work on right-shift functionality.  Added the rest of the
 diagnostics to verify this work.

 Revision 1.138.2.34  2008/04/30 06:07:12  phase1geo
 Adding new diagnostic that will be used to test final portion of right-shift
 functionality.  Checkpointing.

 Revision 1.138.2.33  2008/04/30 05:56:21  phase1geo
 More work on right-shift function.  Added and connected part_select_push and part_select_pull
 functionality.  Also added new right-shift diagnostics.  Checkpointing.

 Revision 1.138.2.32  2008/04/30 04:15:03  phase1geo
 More work on right-shift operator.  Adding diagnostics to verify functionality.
 Checkpointing.

 Revision 1.138.2.31  2008/04/29 22:55:10  phase1geo
 Starting to work on right-shift functionality in vector.c.  Added some new
 diagnostics to regression suite to verify this new code.  More to do.
 Checkpointing.

 Revision 1.138.2.30  2008/04/29 05:50:50  phase1geo
 Created body of right-shift function (although this code is just a copy of the
 left-shift code at the current time).  Checkpointing.

 Revision 1.138.2.29  2008/04/29 05:45:28  phase1geo
 Completing debug and testing of left-shift operator.  Added new diagnostics
 to verify the rest of the functionality.

 Revision 1.138.2.28  2008/04/28 21:08:53  phase1geo
 Fixing memory deallocation issue when CDD file is not present when report
 command is issued.  Fixing issues with left-shift function (still have one
 section to go).  Added new tests to regression suite to verify the new
 left-shift functionality.

 Revision 1.138.2.27  2008/04/28 05:16:35  phase1geo
 Working on initial version of vector_lshift_uint32 functionality.  More testing
 and debugging to do here.  Checkpointing.

 Revision 1.138.2.26  2008/04/26 12:45:07  phase1geo
 Fixing bug in from_string for string types.  Updated regressions.  Checkpointing.

 Revision 1.138.2.25  2008/04/25 17:39:50  phase1geo
 Fixing several vector issues.  Coded up vector_unary_inv and vector_op_negate.
 Starting to update passing regression files.

 Revision 1.138.2.24  2008/04/25 14:12:35  phase1geo
 Coding left shift vector function.  Checkpointing.

 Revision 1.138.2.23  2008/04/25 05:22:46  phase1geo
 Finished restructuring of vector data.  Continuing to test new code.  Checkpointing.

 Revision 1.138.2.22  2008/04/24 23:38:42  phase1geo
 Starting to restructure vector data structure to help with memory access striding issues.
 This work is not complete at this time.  Checkpointing.

 Revision 1.138.2.21  2008/04/24 23:09:40  phase1geo
 Fixing bug 1950946.  Fixing other various bugs in vector code.  Checkpointing.

 Revision 1.138.2.20  2008/04/24 13:20:34  phase1geo
 Completing initial pass of vector_op_subtract function.  Checkpointing.

 Revision 1.138.2.19  2008/04/24 05:51:46  phase1geo
 Added body of subtraction function, although this function does not work at this point.
 Checkpointing.

 Revision 1.138.2.18  2008/04/24 05:23:08  phase1geo
 Fixing various vector-related bugs.  Added vector_op_add functionality.

 Revision 1.138.2.17  2008/04/23 23:06:03  phase1geo
 More bug fixes to vector functionality.  Bitwise operators appear to be
 working correctly when 2-state values are used.  Checkpointing.

 Revision 1.138.2.16  2008/04/23 21:27:06  phase1geo
 Fixing several bugs found in initial testing.  Checkpointing.

 Revision 1.138.2.15  2008/04/23 14:33:50  phase1geo
 Fixing bug in vector display functions that caused infinite looping.  Checkpointing.

 Revision 1.138.2.14  2008/04/23 06:32:32  phase1geo
 Starting to debug vector changes.  Checkpointing.

 Revision 1.138.2.13  2008/04/23 05:20:45  phase1geo
 Completed initial pass of code updates.  I can now begin testing...  Checkpointing.

 Revision 1.138.2.12  2008/04/22 23:01:43  phase1geo
 More updates.  Completed initial pass of expr.c and fsm_arg.c.  Working
 on memory.c.  Checkpointing.

 Revision 1.138.2.11  2008/04/22 14:03:57  phase1geo
 More work on expr.c.  Checkpointing.

 Revision 1.138.2.10  2008/04/22 12:46:29  phase1geo
 More work on expr.c.  Checkpointing.

 Revision 1.138.2.9  2008/04/22 05:51:36  phase1geo
 Continuing work on expr.c.  Checkpointing.

 Revision 1.138.2.8  2008/04/21 23:13:04  phase1geo
 More work to update other files per vector changes.  Currently in the middle
 of updating expr.c.  Checkpointing.

 Revision 1.138.2.7  2008/04/21 04:37:23  phase1geo
 Attempting to get other files (besides vector.c) to compile with new vector
 changes.  Still work to go here.  The initial pass through vector.c is not
 complete at this time as I am attempting to get what I have completed
 debugged.  Checkpointing work.

 Revision 1.138.2.6  2008/04/20 05:43:45  phase1geo
 More work on the vector file.  Completed initial pass of conversion operations,
 bitwise operations and comparison operations.

 Revision 1.138.2.5  2008/04/18 22:04:15  phase1geo
 More work on vector functions for new data structure implementation.  Worked
 on vector_set_value, bit_fill and some checking functions.  Checkpointing.

 Revision 1.138.2.4  2008/04/18 14:14:19  phase1geo
 More vector updates.

 Revision 1.138.2.3  2008/04/18 05:05:28  phase1geo
 More updates to vector file.  Updated merge and output functions.  Checkpointing.

 Revision 1.138.2.2  2008/04/17 23:16:08  phase1geo
 More work on vector.c.  Completed initial pass of vector_db_write/read and
 vector_copy/clone functionality.  Checkpointing.

 Revision 1.138.2.1  2008/04/16 22:29:58  phase1geo
 Finished format for new vector value format.  Completed work on vector_init_uint32
 and vector_create.

 Revision 1.138  2008/04/15 06:08:47  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.137  2008/04/08 19:50:36  phase1geo
 Removing LAST operator for PEDGE, NEDGE and AEDGE expression operations and
 replacing them with the temporary vector solution.

 Revision 1.136  2008/04/08 05:26:34  phase1geo
 Second checkin of performance optimizations (regressions do not pass at this
 point).

 Revision 1.135  2008/04/05 06:19:42  phase1geo
 Fixes memory issues with increment operation and updates to regressions.

 Revision 1.134  2008/04/05 04:49:46  phase1geo
 More regression fixes and updates.

 Revision 1.133  2008/03/31 21:40:24  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.132  2008/03/28 22:04:53  phase1geo
 Fixing calculation of unknown and not_zero supplemental field bits in
 vector_db_write function when X data is being written.  Updated regression
 files accordingly.  Checkpointing.

 Revision 1.131  2008/03/28 21:11:32  phase1geo
 Fixing memory leak issues with -ep option and embedded FSM attributes.

 Revision 1.130  2008/03/28 18:28:26  phase1geo
 Fixing bug in trigger expression function due to recent changes.

 Revision 1.129  2008/03/28 17:27:00  phase1geo
 Fixing expression assignment problem due to recent changes.  Updating
 regression files per changes.

 Revision 1.128  2008/03/27 18:51:46  phase1geo
 Fixing more issues with PASSIGN and BASSIGN operations.

 Revision 1.127  2008/03/27 06:09:58  phase1geo
 Fixing some regression errors.  Checkpointing.

 Revision 1.126  2008/03/26 22:41:07  phase1geo
 More fixes per latest changes.

 Revision 1.125  2008/03/26 21:29:32  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.124  2008/03/24 13:55:46  phase1geo
 More attempts to fix memory issues.  Checkpointing.

 Revision 1.123  2008/03/24 13:16:46  phase1geo
 More changes for memory allocation/deallocation issues.  Things are still pretty
 broke at the moment.

 Revision 1.122  2008/03/22 05:23:24  phase1geo
 More attempts to fix memory problems.  Things are still pretty broke at the moment.

 Revision 1.121  2008/03/18 21:36:24  phase1geo
 Updates from regression runs.  Regressions still do not completely pass at
 this point.  Checkpointing.

 Revision 1.120  2008/03/18 03:56:44  phase1geo
 More updates for memory checking (some "fixes" here as well).

 Revision 1.119  2008/03/17 22:02:32  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.118  2008/03/14 22:00:21  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.117  2008/03/13 10:28:55  phase1geo
 The last of the exception handling modifications.

 Revision 1.116  2008/03/12 03:52:49  phase1geo
 Fixes for regression failures.

 Revision 1.115  2008/03/11 22:06:49  phase1geo
 Finishing first round of exception handling code.

 Revision 1.114  2008/02/10 03:33:13  phase1geo
 More exception handling added and fixed remaining splint errors.

 Revision 1.113  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.112  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

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

