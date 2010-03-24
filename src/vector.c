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
#include <float.h>

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
) { PROFILE(VECTOR_INIT_ULONG);

  vec->width                = width;
  vec->suppl.all            = 0;
  vec->suppl.part.type      = type;
  vec->suppl.part.data_type = VDATA_UL;
  vec->suppl.part.owns_data = owns_value & (width > 0);
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

  new_vec = (vector*)malloc_safe( sizeof( vector ) );

  switch( data_type ) {
    case VDATA_UL :
      {
        ulong** value = NULL;
        if( (data == TRUE) && (width > 0) ) {
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

    assert( vec->width > 0 );

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
                    unsigned int slen;
                    char*        stmp;
                    (*vec)->value.r64->str = strdup_safe( str );
                    slen = strlen( *line );
                    stmp = strdup_safe( *line );
                    *line += chars_read;
                    if( sscanf( remove_underscores( stmp ), "%lf", &((*vec)->value.r64->val)) != 1 ) {
                      free_safe( stmp, (slen + 1) );
                      print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                      Throw 0;
                    }
                    free_safe( stmp, (slen + 1) );
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                    Throw 0;
                  }
                } else {
                  if( sscanf( *line, "%lf%n", &((*vec)->value.r64->val), &chars_read ) == 1 ) {
                    *line += chars_read;
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                    Throw 0;
                  }
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
                    unsigned int slen;
                    char*        stmp;
                    (*vec)->value.r32->str = strdup_safe( str );
                    slen = strlen( *line );
                    stmp = strdup_safe( *line );
                    *line += chars_read;
                    if( sscanf( remove_underscores( stmp ), "%f", &((*vec)->value.r32->val)) != 1 ) {
                      free_safe( stmp, (slen + 1) );
                      print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                      Throw 0;
                    }
                    free_safe( stmp, (slen + 1) );
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                    Throw 0;
                  }
                } else {
                  if( sscanf( *line, "%f%n", &((*vec)->value.r32->val), &chars_read ) == 1 ) {
                    *line += chars_read;
                  } else {
                    print_output( "Unable to parse vector information in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
                    Throw 0;
                  }
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
            int  store_str;
            char value[64];
            if( sscanf( *line, "%d %s%n", &store_str, value, &chars_read ) == 2 ) {
              *line += chars_read;
            } else {
              print_output( "Unable to parse vector information in database file.  Unable to merge.", FATAL, __FILE__, __LINE__ );
              Throw 0;
            }
          }
          break;
        case VDATA_R32 :
          {
            int  store_str;
            char value[64];
            if( sscanf( *line, "%d %s%n", &store_str, value, &chars_read ) == 2 ) {
              *line += chars_read;
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

  if( base->suppl.part.owns_data == 1 ) {

    switch( base->suppl.part.data_type ) {
      case VDATA_UL :
        for( i=0; i<UL_SIZE(base->width); i++ ) {
          for( j=2; j<vector_type_sizes[base->suppl.part.type]; j++ ) {
            base->value.ul[i][j] |= other->value.ul[i][j];
          }
        }
        break;
      case VDATA_R64 :
      case VDATA_R32 :
        /* Nothing to do here */
        break;
      default :  assert( 0 );  break;
    }

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
 Displays the binary value of the specified ulong values to standard output.
*/
static void vector_display_value_ulongs(
  ulong* vall,  /*!< Array of lower values */
  ulong* valh,  /*!< Array of upper values */
  int    width  /*!< Number of bits to display */
) {

  int i, j;  /* Loop iterator */
  int bits_left = UL_MOD(width - 1);

  printf( "value: %d'b", width );
  
  for( i=UL_SIZE(width); i--; ) {
    for( j=bits_left; j>=0; j-- ) {
      if( ((valh[i] >> (unsigned int)j) & 0x1) == 0 ) {
        printf( "%lu", ((vall[i] >> (unsigned int)j) & 0x1) );
      } else {
        if( ((vall[i] >> (unsigned int)j) & 0x1) == 0 ) {
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

  printf( "read value: %s, stored value: %.16lf", value->str, value->val );

}

/*!
 Outputs the contents of a 32-bit real vector value.
*/
void vector_display_r32(
  rv32* value  /*!< Pointer to real32 structure from vector */
) {

  printf( "read value: %s, stored value: %.16f", value->str, value->val );

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
      case VDATA_R32 :  vector_display_r32( vec->value.r32 );  break;
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
          ulong xmask = mask & ~tvalh;
          if( prev_set == 1 ) {
            entry[VTYPE_INDEX_SIG_TOG01] |= ((~tvalh & ~tvall) | (tvalx & ~xval)) & (~fvalh &  fvall) & mask;
            entry[VTYPE_INDEX_SIG_TOG10] |= ((~tvalh &  tvall) | (tvalx &  xval)) & (~fvalh & ~fvall) & mask;
          }
          entry[VTYPE_INDEX_SIG_VALL]  = (tvall & ~mask)  | fvall;
          entry[VTYPE_INDEX_SIG_VALH]  = (tvalh & ~mask)  | fvalh;
          entry[VTYPE_INDEX_SIG_XHOLD] = (xval  & ~xmask) | (tvall & xmask);
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
          ulong xmask = mask & ~tvalh;
          entry[VTYPE_INDEX_MEM_TOG01] |= ((~tvalh & ~tvall) | (tvalx & ~xval)) & (~fvalh &  fvall) & mask;
          entry[VTYPE_INDEX_MEM_TOG10] |= ((~tvalh &  tvall) | (tvalx &  xval)) & (~fvalh & ~fvall) & mask;
          entry[VTYPE_INDEX_MEM_WR]    |= mask;
          entry[VTYPE_INDEX_MEM_VALL]   = (tvall & ~mask)  | fvall;
          entry[VTYPE_INDEX_MEM_VALH]   = (tvalh & ~mask)  | fvalh;
          entry[VTYPE_INDEX_MEM_XHOLD]  = (xval  & ~xmask) | (tvall & xmask);
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
  int           msb,   /*!< MSB offset */
  bool          xfill  /*!< If set to TRUE, causes lower bits to be X filled */
) { PROFILE(VECTOR_LSHIFT_ULONG);

  int          from_msb = (msb - lsb);
  unsigned int diff     = UL_DIV(msb) - UL_DIV(from_msb);

  if( UL_DIV(lsb) == UL_DIV(msb) ) {

    int   i;
    ulong uset = xfill ? ~UL_LMASK(lsb) : 0;
    
    vall[diff] = (vec->value.ul[0][VTYPE_INDEX_VAL_VALL] << (unsigned int)lsb);
    valh[diff] = (vec->value.ul[0][VTYPE_INDEX_VAL_VALH] << (unsigned int)lsb) | uset;

    for( i=(diff-1); i>=0; i-- ) {
      vall[i] = 0;
      valh[i] = xfill ? UL_SET : 0;
    }

  } else {
  
    int  i;
    bool use_vec = msb > (vec->width - 1);
    int  hindex  = use_vec ? UL_DIV(vec->width - 1) : UL_DIV(msb);
    
    /* Transfer the vector value to the val array */
    for( i=0; i<=hindex; i++ ) {
      vall[i] = vec->value.ul[i][VTYPE_INDEX_VAL_VALL];
      valh[i] = vec->value.ul[i][VTYPE_INDEX_VAL_VALH];
    }
    
    if( use_vec ) {

      ulong uset1 = xfill ? ~UL_HMASK(vec->width - 1) : 0;
      ulong uset2 = xfill ? UL_SET : 0;
            
      valh[i-1] = valh[i-1] | uset1;

      for( ; i<=UL_DIV(msb); i++ ) {
        vall[i] = 0;
        valh[i] = uset2;
      }

    }

    if( UL_MOD(lsb) == 0 ) {

      int i;
    
      for( i=UL_DIV(from_msb); i>=0; i-- ) {
        vall[i+diff] = vall[i];
        valh[i+diff] = valh[i];
      }

      for( i=(diff-1); i>=0; i-- ) {
        vall[i] = 0;
        valh[i] = xfill ? UL_SET : 0;
      }

    } else if( UL_MOD(msb) > UL_MOD(from_msb) ) {

      unsigned int mask_bits1  = UL_MOD(from_msb + 1);
      unsigned int shift_bits1 = UL_MOD(msb) - UL_MOD(from_msb);
      ulong        mask1       = UL_SET >> (UL_BITS - mask_bits1);
      ulong        mask2       = UL_SET << (UL_BITS - shift_bits1);
      ulong        mask3       = ~mask2;
      ulong        uset        = xfill ? ~UL_LMASK(lsb) : 0;
      int          i;
      
      vall[UL_DIV(msb)] = (vall[UL_DIV(from_msb)] & mask1) << shift_bits1;
      valh[UL_DIV(msb)] = (valh[UL_DIV(from_msb)] & mask1) << shift_bits1;

      for( i=(UL_DIV(from_msb) - 1); i>=0; i-- ) {
        vall[i+diff+1] |= ((vall[i] & mask2) >> (UL_BITS - shift_bits1));
        valh[i+diff+1] |= ((valh[i] & mask2) >> (UL_BITS - shift_bits1));
        vall[i+diff]    = ((vall[i] & mask3) << shift_bits1);
        valh[i+diff]    = ((valh[i] & mask3) << shift_bits1);
      }

      valh[diff] |= uset;

      for( i=(diff - 1); i>=0; i-- ) {
        vall[i] = 0;
        valh[i] = xfill ? UL_SET : 0;
      }

    } else {

      unsigned int mask_bits1  = UL_MOD(from_msb);
      unsigned int shift_bits1 = mask_bits1 - UL_MOD(msb);
      ulong        mask1       = UL_SET << mask_bits1;
      ulong        mask2       = UL_SET >> (UL_BITS - shift_bits1);
      ulong        mask3       = ~mask2;
      ulong        uset        = xfill ? ~UL_LMASK(lsb) : 0;
      int          i;
            
      vall[UL_DIV(msb)] = (vall[UL_DIV(from_msb)] & mask3) >> shift_bits1;
      valh[UL_DIV(msb)] = (valh[UL_DIV(from_msb)] & mask3) >> shift_bits1;

      for( i=UL_DIV(from_msb); i>=0; i-- ) {
        vall[(i+diff)-1] = ((vall[i] & mask2) << (UL_BITS - shift_bits1));
        valh[(i+diff)-1] = ((valh[i] & mask2) << (UL_BITS - shift_bits1));
        if( i > 0 ) {
          vall[(i+diff)-1] |= ((vall[i-1] & mask3) >> shift_bits1);
          valh[(i+diff)-1] |= ((valh[i-1] & mask3) >> shift_bits1);
        }
      }

      valh[diff-1] |= uset;

      for( i=(diff - 2); i>=0; i-- ) {
        vall[i] = 0;
        valh[i] = xfill ? UL_SET : 0;
      }
      
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
  int           msb,   /*!< MSB of vec range to shift */
  bool          xfill  /*!< Set to TRUE if the upper bits should be X filled or not */
) { PROFILE(VECTOR_RSHIFT_ULONG);

  int          adj_lsb = (lsb < 0) ? 0 : lsb;
  int          diff    = UL_DIV(adj_lsb);
  unsigned int rwidth  = (msb - adj_lsb) + 1;
  
  if( adj_lsb > msb ) {

    unsigned int i;

    for( i=0; i<UL_SIZE( vec->width ); i++ ) {
      vall[i] = 0;
      valh[i] = xfill ? UL_SET : 0;
    }

  } else if( lsb < 0 ) {

    /* First, left shift the data */
    vector_lshift_ulong( vec, vall, valh, (0 - lsb), (msb + (0 - lsb)), xfill ); 

  } else {

    bool use_vec = msb > (vec->width - 1);
    int  hindex  = use_vec ? UL_DIV(vec->width - 1) : UL_DIV(msb);
    int  i;
    int  start_i;
    
    /* Transfer the vector value to the val array */
    for( i=0; i<=hindex; i++ ) {
      vall[i] = vec->value.ul[i][VTYPE_INDEX_VAL_VALL];
      valh[i] = vec->value.ul[i][VTYPE_INDEX_VAL_VALH];
    }
    
    if( use_vec ) {

      ulong uset1 = xfill ? ~UL_HMASK(vec->width - 1) : 0;
      ulong uset2 = xfill ? UL_SET : 0;
            
      valh[i-1] = valh[i-1] | uset1;
      
      for( ; i<=UL_DIV(msb); i++ ) {
        vall[i] = 0;
        valh[i] = uset2;
      }

    }
    
    if( UL_DIV(adj_lsb) == UL_DIV(msb) ) {

      vall[0] = (vall[diff] >> UL_MOD(adj_lsb));
      valh[0] = (valh[diff] >> UL_MOD(adj_lsb));
      start_i = 1;

    } else if( UL_MOD(adj_lsb) == 0 ) {

      unsigned int i;
      ulong        lmask = UL_HMASK(msb);

      for( i=diff; i<UL_DIV(msb); i++ ) {
        vall[i-diff] = vall[i];
        valh[i-diff] = valh[i];
      }
      vall[i-diff] = vall[i] & lmask;
      valh[i-diff] = valh[i] & lmask;
      start_i      = (i - diff) + 1;

    } else {

      unsigned int shift_bits = UL_MOD(adj_lsb);
      ulong        mask1      = UL_SET << shift_bits;
      ulong        mask2      = ~mask1;
      ulong        mask3      = UL_SET >> ((UL_BITS - 1) - UL_MOD(msb - adj_lsb));
      unsigned int hindex     = UL_DIV(msb);
      unsigned int i;

      for( i=0; i<=UL_DIV(rwidth - 1); i++ ) {
        vall[i]  = (vall[i+diff] & mask1) >> shift_bits;
        valh[i]  = (valh[i+diff] & mask1) >> shift_bits;
        if( (i+diff+1) <= hindex ) {
          vall[i] |= (vall[i+diff+1] & mask2) << (UL_BITS - shift_bits);
          valh[i] |= (valh[i+diff+1] & mask2) << (UL_BITS - shift_bits);
        }
      }

      vall[i-1] &= mask3;
      valh[i-1] &= mask3;
      start_i    = i;

    }
        
    /* Bit-fill with zeroes or X */
    for( ; start_i<=UL_SIZE( vec->width ); start_i++ ) {
      vall[start_i] = 0;
      valh[start_i] = xfill ? UL_SET : 0;
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
 Sets the memory read bits of the given vector for the given bit range.
*/
void vector_set_mem_rd_ulong(
  vector* vec,  /*!< Pointer to vector that will set the memory read */
  int     msb,  /*!< MSB offset */
  int     lsb   /*!< LSB offset */
) { PROFILE(VECTOR_SET_MEM_RD);

  if( vec->suppl.part.type == VTYPE_MEM ) {
    if( UL_DIV(msb) == UL_DIV(lsb) ) {
      vec->value.ul[UL_DIV(lsb)][VTYPE_INDEX_MEM_RD] |= UL_HMASK(msb) & UL_LMASK(lsb);
    } else {
      int i;
      vec->value.ul[UL_DIV(lsb)][VTYPE_INDEX_MEM_RD] |= UL_LMASK(lsb);
      for( i=(UL_DIV(lsb) + 1); i<UL_DIV(msb); i++ ) {
        vec->value.ul[UL_DIV(msb)][VTYPE_INDEX_MEM_RD] = UL_SET;
      }
      vec->value.ul[UL_DIV(msb)][VTYPE_INDEX_MEM_RD] |= UL_HMASK(msb);
    } 
  }

  PROFILE_END;

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
        vector_rshift_ulong( src, vall, valh, lsb, msb, TRUE );

        /* If the src vector is of type MEM, set the MEM_RD bit in the source's supplemental field */
        if( set_mem_rd ) {
          vector_set_mem_rd_ulong( src, msb, lsb );
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
            vector_lshift_ulong( src, vall, valh, diff, ((src_msb - src_lsb) + diff), FALSE );
          /* Otherwise, right-shift the source vector to match up */
          } else {
            diff = (src_lsb - tgt_lsb);
            vector_rshift_ulong( src, vall, valh, diff, ((src_msb - src_lsb) + diff), FALSE );
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

  if( vec->value.ul != NULL ) {
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
  } else {
    size = 1;
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

  if( vec->value.ul != NULL ) {
    switch( vec->suppl.part.data_type ) {
      case VDATA_UL :
        size = UL_SIZE( vec->width );
        while( (i < size) && (vec->value.ul[i][VTYPE_INDEX_VAL_VALL] == 0) ) i++;
        break;
      case VDATA_R64 :
        size = DEQ( vec->value.r64->val, 0.0 ) ? 1 : 0;
        break;
      case VDATA_R32 :
        size = FEQ( vec->value.r32->val, 0.0) ? 1 : 0;
        break;
      default :  assert( 0 );  break;
    }
  } else {
    size = 1;
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
    case VDATA_R64 :  retval = (int)round( vec->value.r64->val );       break;
    case VDATA_R32 :  retval = (int)roundf( vec->value.r32->val );       break;
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
      if( (vec->width > 32) && (sizeof( ulong ) == 4) ) {
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
 \return Returns TRUE if the value has changed from the last assignment to the given vector.

 Converts an integer value into a vector, creating a vector value to store the
 new vector into.  This function is used along with the vector_to_int for
 mathematical vector operations.  We will first convert vectors into integers,
 perform the mathematical operation, and then revert the integers back into
 the vectors.
*/
bool vector_from_int(
  vector* vec,   /*!< Pointer to vector store value into */
  int     value  /*!< Integer value to convert into vector */
) { PROFILE(VECTOR_FROM_INT);

  bool retval = TRUE;  /* Return value for this function */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        scratchl[UL_DIV(MAX_BIT_WIDTH)];
        ulong        scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int i;
        unsigned int size        = (vec->width < (sizeof( int ) << 3)) ? UL_SIZE( vec->width ) : UL_SIZE( sizeof( int ) << 3 );
        bool         sign_extend = (value < 0) && (vec->width > (sizeof( int ) << 3));
        unsigned int shift       = (UL_BITS <= (sizeof( int ) << 3)) ? UL_BITS : (sizeof( int ) << 3);
        for( i=0; i<size; i++ ) {
          scratchl[i] = (ulong)value & UL_SET;
          scratchh[i] = 0;
          /*@-shiftimplementation@*/
          value >>= shift;
          /*@=shiftimplementation@*/
        }
        if( sign_extend ) {
          vector_sign_extend_ulong( scratchl, scratchh, UL_SET, UL_SET, (vec->width - 1), vec->width );
        } else {
          size = UL_SIZE( vec->width );
          for( ; i<size; i++ ) {
            scratchl[i] = scratchh[i] = 0;
          }
        }
        retval = vector_set_coverage_and_assign_ulong( vec, scratchl, scratchh, 0, (vec->width - 1) );
      }
      break;
    case VDATA_R64 :
      retval              = !DEQ( vec->value.r64->val, (double)value );
      vec->value.r64->val = (double)value;
      break;
    case VDATA_R32 :
      retval              = !FEQ( vec->value.r32->val, (float)value );
      vec->value.r32->val = (float)value;
      break;
    default :  assert( 0 );  break;
  }

  /* Because this value came from an integer, specify that the vector is signed */
  vec->suppl.part.is_signed = 1;

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the value has changed from the last assignment to the given vector.

 Converts a 64-bit integer value into a vector.  This function is used along with
 the vector_to_uint64 for mathematical vector operations.  We will first convert
 vectors into 64-bit integers, perform the mathematical operation, and then revert
 the 64-bit integers back into the vectors.
*/
bool vector_from_uint64(
  vector* vec,   /*!< Pointer to vector store value into */
  uint64  value  /*!< 64-bit integer value to convert into vector */
) { PROFILE(VECTOR_FROM_UINT64);

  bool retval = TRUE;  /* Return value for this function */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      {
        ulong        scratchl[UL_DIV(MAX_BIT_WIDTH)];
        ulong        scratchh[UL_DIV(MAX_BIT_WIDTH)];
        unsigned int i;
        unsigned int size  = (vec->width < (sizeof( uint64 ) << 3)) ? UL_SIZE( vec->width ) : UL_SIZE( sizeof( uint64 ) << 3 );
        unsigned int shift = (UL_BITS <= (sizeof( uint64 ) << 3)) ? UL_BITS : (sizeof( uint64 ) << 3); 
        for( i=0; i<size; i++ ) {
          scratchl[i] = (ulong)value & UL_SET;
          scratchh[i] = 0;
          value >>= shift;
        }
        retval = vector_set_coverage_and_assign_ulong( vec, scratchl, scratchh, 0, (vec->width - 1) );
      }
      break;
    case VDATA_R64 :
      retval              = !DEQ( vec->value.r64->val, (double)value );
      vec->value.r64->val = (double)value;
      break;
    case VDATA_R32 :
      retval              = !FEQ( vec->value.r32->val, (float)value );
      vec->value.r32->val = (float)value;
      break;
    default :  assert( 0 );  break;
  }

  /* Because this value came from an unsigned integer, specify that the vector is unsigned */
  vec->suppl.part.is_signed = 0;

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the value has changed from the last assignment to the given vector.

 Converts a 64-bit real value into a vector.
*/
bool vector_from_real64(
  vector* vec,   /*!< Pointer to vector store value into */
  real64  value  /*!< 64-bit real value to convert into vector */
) { PROFILE(VECTOR_FROM_REAL64);

  bool retval = TRUE;  /* Return value for this function */

  switch( vec->suppl.part.data_type ) {
    case VDATA_UL :
      retval = vector_from_uint64( vec, (uint64)round( value ) );
      break;
    case VDATA_R64 :
      retval              = !DEQ( vec->value.r64->val, value );
      vec->value.r64->val = value;
      break;
    case VDATA_R32 :
      retval              = !FEQ( vec->value.r32->val, (float)value);
      vec->value.r32->val = (float)value;
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

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
  vector*      vec,       /*!< Pointer to vector to convert */
  int          base,      /*!< Base type of vector value */
  bool         show_all,  /*!< Set to TRUE causes all bits in vector to be displayed (otherwise, only significant bits are displayed) */
  unsigned int width      /*!< If set to a value greater than 0, uses this width to output instead of the supplied vector */
) { PROFILE(VECTOR_TO_STRING);

  char* str = NULL;  /* Pointer to allocated string */

  /* Calculate the width to output */
  if( (width == 0) || (vec->width < width) ) {
    width = vec->width;
  }

  if( base == QSTRING ) {

    int i, j;
    int vec_size = ((width - 1) >> 3) + 2;
    int pos      = 0;

    /* Allocate memory for string from the heap */
    str = (char*)malloc_safe( vec_size );

    switch( vec->suppl.part.data_type ) {
      case VDATA_UL :
        {
          int offset = (((width >> 3) & (UL_MOD_VAL >> 3)) == 0) ? SIZEOF_LONG : ((width >> 3) & (UL_MOD_VAL >> 3));
          for( i=UL_SIZE(width); i--; ) {
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

  } else if( base == SIZED_DECIMAL ) {

    char         width_str[50];
    unsigned int rv = snprintf( width_str, 50, "%u'd%d", width, vector_to_int( vec ) );
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
    int          vec_size  = ((width - 1) >> 3) + 2;
    int          pos       = 0;

    switch( base ) {
      case BINARY :  
        vec_size  = (width + 1);
        group     = 1;
        type_char = 'b';
        break;
      case OCTAL :  
        vec_size  = ((width % 3) == 0) ? ((width / 3) + 1)
                                       : ((width / 3) + 2);
        group     = 3;
        type_char = 'o';
        break;
      case HEXIDECIMAL :  
        vec_size  = ((width % 4) == 0) ? ((width / 4) + 1)
                                       : ((width / 4) + 2);
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
          for( i=(width - 1); i>=0; i-- ) {
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

    rv = snprintf( width_str, 20, "%u", width );
    assert( rv < 20 );
    str_size = strlen( width_str ) + 2 + strlen( tmp ) + 1 + vec->suppl.part.is_signed;
    str      = (char*)malloc_safe( str_size );
    if( vec->suppl.part.is_signed == 0 ) {
      rv = snprintf( str, str_size, "%u'%c%s", width, type_char, tmp );
    } else {
      rv = snprintf( str, str_size, "%u's%c%s", width, type_char, tmp );
    }
    assert( rv < str_size );

    free_safe( tmp, vec_size );

  }

  PROFILE_END;

  return( str );

}

/*!
 Stores the specified string into the specified vector.  If the entire string cannot fit in the vector, only store as many
 characters as is possible.
*/
void vector_from_string_fixed(
  vector*     vec,  /*!< Pointer to vector to store string value */
  const char* str   /*!< Pointer to string value to store */
) { PROFILE(VECTOR_FROM_STRING_FIXED);

  unsigned int width = ((vec->width >> 3) < strlen( str )) ? (vec->width >> 3) : strlen( str );
  unsigned int pos = 0;
  int          i;

  /* TBD - Not sure if I need to support both endianness values here */
  for( i=(width-1); i>=0; i-- ) {
    vec->value.ul[pos>>(UL_DIV_VAL-3)][VTYPE_INDEX_VAL_VALL] |= (ulong)(str[i]) << ((pos & (UL_MOD_VAL >> 3)) << 3);
    pos++;
  }

  PROFILE_END;

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

  int    bits_per_char;         /* Number of bits represented by a single character in the value string str */
  int    size;                  /* Specifies bit width of vector to create */
  char   value[MAX_BIT_WIDTH];  /* String to store string value in */
  char   stype[3];              /* Temporary holder for type of string being parsed */
  int    chars_read;            /* Number of characters read by a sscanf() function call */
  double real;                  /* Container for real number */

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

    unsigned int slen = strlen( *str );
    char*        stmp = strdup_safe( *str );

    if( ((sscanf( *str, "%[0-9_]%[.]%[0-9_]", value, value, value ) == 3) ||
         (sscanf( *str, "-%[0-9_]%[.]%[0-9_]", value, value, value ) == 3)) &&
        (sscanf( remove_underscores( stmp ), "%lf%n", &real, &chars_read ) == 1) ) {

      *vec                         = vector_create( 64, VTYPE_VAL, VDATA_R64, TRUE );
      (*vec)->value.r64->val       = real;
      (*vec)->value.r64->str       = strdup_safe( *str );
      (*vec)->suppl.part.is_signed = 1;
      *str                         = *str + chars_read;

    } else {

      if( sscanf( *str, "%d'%[sSdD]%[0-9]%n", &size, stype, value, &chars_read ) == 3 ) {
        bits_per_char = 10;
        *base         = SIZED_DECIMAL;
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
      } else {
        /* If the specified string is none of the above, return NULL */
        bits_per_char = 0;
      }

      /* If we have exceeded the maximum number of bits, return a value of NULL */
      if( (size > MAX_BIT_WIDTH) || (bits_per_char == 0) ) {

        *vec  = NULL;
        *base = 0;

      } else {

        /* Create vector */
        *vec = vector_create( size, VTYPE_VAL, VDATA_UL, TRUE );
        if( (*base == DECIMAL) || (*base == SIZED_DECIMAL) ) {
          (void)vector_from_uint64( *vec, ato64( value ) );
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

    /* Deallocate memory */
    free_safe( stmp, (slen + 1) );

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

  /* Make adjust ment to MSB if necessary */
  msb = (msb > 0) ? msb : -msb;

  assert( vec != NULL );
  assert( value != NULL );
  assert( msb <= vec->width );

  /* Set pointer to LSB */
  ptr = (value + strlen( value )) - 1;
  i   = lsb;

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
      {
        double real;
        if( sscanf( value, "%lf", &real ) == 1 ) {
          retval = !DEQ( vec->value.r64->val, real );
          vec->value.r64->val = real;
        }
      }
      break;
    case VDATA_R32 :
      {
        float real;
        if( sscanf( value, "%f", &real ) != 1 ) {
          retval = !FEQ( vec->value.r32->val, real );
          vec->value.r32->val = real;
        }
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
 \return Returns TRUE if the value has changed from the previous value.

 Assigns the given vectors the value from the VCD file (bit-extended as necessary).
*/
bool vector_vcd_assign2(
               vector* vec1,   /*!< Vector to fill which occupies the low-order bits */
               vector* vec2,   /*!< Vector to fill which occupies the high-order bits */
               char*   value,  /*!< VCD value (in string form) */
  /*@unused@*/ int     msb,    /*!< Most-significant bit from VCD file */
  /*@unused@*/ int     lsb     /*!< Least-significant bit from VCD file */
) { PROFILE(VECTOR_VCD_ASSIGN2);

  bool         retval     = FALSE;
  unsigned int value_size = strlen( value );

  /* If the value string is greater than the low-order vector, split the string and perform two individual VCD assigns */
  if( value_size > vec1->width ) {

    char* ptr = value + (value_size - vec1->width);

    retval |= vector_vcd_assign( vec1, ptr, (vec1->width - 1), 0 );
    *ptr = '\0';
    retval |= vector_vcd_assign( vec2, value, (vec2->width - 1), 0 );

  /* Otherwise, assign the low-order vector as normal and assign the high-order vector with only the first character of the value string */
  } else {

    retval |= vector_vcd_assign( vec1, value, (vec1->width - 1), 0 );
    if( value[0] == '1' ) {
      value[0] = '0';
    }
    value[1] = '\0';
    retval |= vector_vcd_assign( vec2, value, (vec2->width - 1), 0 );

  }

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
            scratchl = DEQ( vector_to_real64( left ), vector_to_real64( right )) ? 1 : 0;
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
            scratchl = !DEQ( vector_to_real64( left ), vector_to_real64( right ) ) ? 1 : 0;
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
          vector_lshift_ulong( left, vall, valh, shift_val, ((left->width + shift_val) - 1), FALSE );
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
          vector_rshift_ulong( left, vall, valh, shift_val, (left->width - 1), FALSE );
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

          vector_rshift_ulong( left, vall, valh, shift_val, msb, FALSE );
          if( left->suppl.part.is_signed ) {
            vector_get_sign_extend_vector_ulong( left, &signl, &signh );
            vector_sign_extend_ulong( vall, valh, signl, signh, (msb - shift_val), tgt->width );
          }
        
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
          retval              = !DEQ( tgt->value.r64->val, result );
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) + vector_to_real64( right ));
          retval              = !FEQ( tgt->value.r32->val, result );
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
          retval              = !DEQ( tgt->value.r64->val, result );
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(0.0 - vector_to_real64( src ));
          retval              = !FEQ( tgt->value.r32->val, result );
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
          retval              = !DEQ( tgt->value.r64->val, result );
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) - vector_to_real64( right ));
          retval              = !FEQ( tgt->value.r32->val, result );
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
          retval              = !DEQ( tgt->value.r64->val, result );
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) * vector_to_real64( right ));
          retval              = !FEQ( tgt->value.r32->val, result );
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
          retval              = !DEQ( tgt->value.r64->val, result);
          tgt->value.r64->val = result;
        }
        break;
      case VDATA_R32 :
        {
          float result        = (float)(vector_to_real64( left ) / vector_to_real64( right ));
          retval              = !FEQ( tgt->value.r32->val, result );
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
        unsigned int rsize  = (pos == 0) ? 0 : UL_SIZE( pos );

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
 \return Returns TRUE if the new value differs from the old value; otherwise, returns FALSE.

 Performs clog2 operation.
*/
bool vector_op_clog2(
  vector*       tgt,  /*!< Pointer to vector to store results in */
  const vector* src   /*! Pointer to value to perform operation on */
) { PROFILE(VECTOR_OP_CLOG2);

  bool  retval;
  ulong vall = 0;
  ulong valh = 0;

  if( vector_is_unknown( src ) ) {

    retval = vector_set_to_x( tgt );

  } else {

    switch( src->suppl.part.data_type ) {
      case VDATA_UL :
        {
          unsigned int size     = UL_SIZE(src->width);
          unsigned int num_ones = 0;
          while( size > 0 ) {
            ulong i = src->value.ul[--size][VTYPE_INDEX_VAL_VALL];
            while( i != 0 ) {
              vall++;
              num_ones += (i & 0x1);
              i >>= 1;
            }
            if( vall != 0 ) {
              vall += (size * UL_BITS);
              if( num_ones == 1 ) {
                while( (size > 0) && (src->value.ul[--size][VTYPE_INDEX_VAL_VALL] == 0) );
                if( size == 0 ) {
                  vall--;
                }
              }
              break;
            }
          }
        }
        break;
      case VDATA_R64 :
      case VDATA_R32 :
        {
          uint64       i        = vector_to_uint64( src ) - 1;
          unsigned int num_ones = 0;
          while( i != 0 ) {
            vall++;
            num_ones += (i & 0x1);
            i >>= 1;
          }
          if( num_ones == 1 ) {
            vall--;
          }
        }
        break;
      default :  assert( 0 );  break;
    } 

    retval = vector_set_coverage_and_assign_ulong( tgt, &vall, &valh, 0, (tgt->width - 1) );

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
      if( vec->width > 0 ) {
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

