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
 \file     reentrant.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/11/2006
*/

#include <assert.h>

#include "defines.h"
#include "reentrant.h"
#include "util.h"


extern const exp_info exp_op_info[EXP_OP_NUM];


/*!
 \return Returns the total number of bits in all signals in this functional unit and all parents
         within the same reentrant task/function.

 Recursively iterates up the functional unit tree keeping track of the total number of bits needed
 to store all information in the current reentrant task/function.
*/
static int reentrant_count_afu_bits(
  func_unit* funit  /*!< Pointer to current function to count the bits of */
) { PROFILE(REENTRANT_COUNT_AFU_BITS);

  int bits = 0;  /* Number of bits in this functional unit and all parent functional units in the reentrant task/function */

  if( (funit->suppl.part.type == FUNIT_ATASK) || (funit->suppl.part.type == FUNIT_AFUNCTION) || (funit->suppl.part.type == FUNIT_ANAMED_BLOCK) ) {

    unsigned int i;

    /* Count the number of signal bits in this functional unit */
    for( i=0; i<funit->sig_size; i++ ) {
      vsignal* sig = funit->sigs[i];
      switch( sig->value->suppl.part.data_type ) {
        case VDATA_UL  :  bits += (sig->value->width * 2) + 1;  break;
        case VDATA_R64 :  bits += 64;  break;
        case VDATA_R32 :  bits += 32;  break;
        default        :  assert( 0 );  break;
      }
    }

    /* Count the number of expression bits in this functional unit */
    for( i=0; i<funit->exp_size; i++ ) {
      expression* exp = funit->exps[i];
      if( (EXPR_OWNS_VEC( exp->op ) == 1) && (EXPR_IS_STATIC( exp ) == 0) ) {
        bits += (exp->value->width * 2);
      }
      bits += ((ESUPPL_BITS_TO_STORE % 2) == 0) ? ESUPPL_BITS_TO_STORE : (ESUPPL_BITS_TO_STORE + 1);
    }

    /* If the current functional unit is a named block, gather the bits in the parent functional unit */
    if( funit->suppl.part.type == FUNIT_ANAMED_BLOCK ) {
      bits += reentrant_count_afu_bits( funit->parent );
    }

  }

  PROFILE_END;

  return( bits );

}

/*!
 Recursively gathers all signal data bits to store and stores them in the given reentrant
 structure.
*/
static void reentrant_store_data_bits(
  func_unit*   funit,    /*!< Pointer to current functional unit to traverse */
  reentrant*   ren,      /*!< Pointer to reentrant structure to populate */
  unsigned int curr_bit  /*!< Current bit to store (should be started at a value of 0) */
) { PROFILE(REENTRANT_STORE_DATA_BITS);

  if( (funit->suppl.part.type == FUNIT_ATASK) || (funit->suppl.part.type == FUNIT_AFUNCTION) || (funit->suppl.part.type == FUNIT_ANAMED_BLOCK) ) {

    unsigned int i;

    /* Walk through the signal list in the reentrant functional unit, compressing and saving vector values */
    for( i=0; i<funit->sig_size; i++ ) {
      vsignal* sig = funit->sigs[i];
      switch( sig->value->suppl.part.data_type ) {
        case VDATA_UL :
          {
            unsigned int i;
            for( i=0; i<sig->value->width; i++ ) {
              ulong* entry = sig->value->value.ul[UL_DIV(i)];
              ren->data[curr_bit>>3] |= (((entry[VTYPE_INDEX_VAL_VALL] >> UL_MOD(i)) & 0x1) << (curr_bit & 0x7));
              curr_bit++;
              ren->data[curr_bit>>3] |= (((entry[VTYPE_INDEX_VAL_VALH] >> UL_MOD(i)) & 0x1) << (curr_bit & 0x7));
              curr_bit++;
            }
            ren->data[curr_bit>>3] |= sig->value->suppl.part.set << (curr_bit & 0x7);
            curr_bit++;
            /* Clear the set bit */
            sig->value->suppl.part.set = 0;
          }
          break;
        case VDATA_R64 :
          {
            uint64       real_bits = sys_task_realtobits( sig->value->value.r64->val );
            unsigned int i;
            for( i=0; i<64; i++ ) {
              ren->data[curr_bit>>3] |= (real_bits & 0x1) << (curr_bit & 0x7);
              real_bits >>= 1;
              curr_bit++;
            }
          }
          break;
        case VDATA_R32 :
          {
            uint64       real_bits = sys_task_realtobits( (double)sig->value->value.r32->val );
            unsigned int i;
            for( i=0; i<32; i++ ) {
              ren->data[curr_bit>>3] |= (real_bits & 0x1) << (curr_bit & 0x7);
              real_bits >>= 1;
              curr_bit++;
            }
          }
          break;
        default :  assert( 0 );
      }
    }

    /* Walk through expression list in the reentrant functional unit, compressing and saving vector and supplemental values */
    for( i=0; i<funit->exp_size; i++ ) {
      expression*  exp = funit->exps[i];
      unsigned int j;
      if( (EXPR_OWNS_VEC( exp->op ) == 1) && (EXPR_IS_STATIC( exp ) == 0) ) {
        switch( exp->value->suppl.part.data_type ) {
          case VDATA_UL :
            {
              for( j=0; j<exp->value->width; j++ ) {
                ulong* entry = exp->value->value.ul[UL_DIV(j)];
                ren->data[curr_bit>>3] |= (((entry[VTYPE_INDEX_VAL_VALL] >> UL_MOD(j)) & 0x1) << (curr_bit & 0x7));
                curr_bit++;
                ren->data[curr_bit>>3] |= (((entry[VTYPE_INDEX_VAL_VALH] >> UL_MOD(j)) & 0x1) << (curr_bit & 0x7));
                curr_bit++;
              }
            }
            break;
          case VDATA_R64 :
            {
              uint64 real_bits = sys_task_realtobits( exp->value->value.r64->val );
              for( j=0; j<64; j++ ) {
                ren->data[curr_bit>>3] |= (real_bits & 0x1) << (curr_bit & 0x7);
                real_bits >>= 1;
                curr_bit++;
              }
            }
            break;
          case VDATA_R32 :
            {
              uint64 real_bits = sys_task_realtobits( (double)exp->value->value.r32->val );
              for( j=0; j<32; j++ ) {
                ren->data[curr_bit>>3] |= (real_bits & 0x1) << (curr_bit & 0x7);
                real_bits >>= 1;
                curr_bit++;
              }
            }
            break;
          default :  assert( 0 );  break;
        }
      }
      for( j=0; j<(((ESUPPL_BITS_TO_STORE % 2) == 0) ? ESUPPL_BITS_TO_STORE : (ESUPPL_BITS_TO_STORE + 1)); j++ ) {
        switch( j ) {
          case 0 :  ren->data[curr_bit>>3] |= (exp->suppl.part.left_changed  << (curr_bit & 0x7));  break;
          case 1 :  ren->data[curr_bit>>3] |= (exp->suppl.part.right_changed << (curr_bit & 0x7));  break;
          case 2 :  ren->data[curr_bit>>3] |= (exp->suppl.part.eval_t        << (curr_bit & 0x7));  break;
          case 3 :  ren->data[curr_bit>>3] |= (exp->suppl.part.eval_f        << (curr_bit & 0x7));  break;
          case 4 :  ren->data[curr_bit>>3] |= (exp->suppl.part.prev_called   << (curr_bit & 0x7));  break;
        }
        curr_bit++;
      }
      /* Clear supplemental bits that have been saved off */
      exp->suppl.part.left_changed  = 0;
      exp->suppl.part.right_changed = 0;
      exp->suppl.part.eval_t        = 0;
      exp->suppl.part.eval_f        = 0;
      exp->suppl.part.prev_called   = 0;
    }

    /* If the current functional unit is a named block, store the bits in the parent functional unit */
    if( funit->suppl.part.type == FUNIT_ANAMED_BLOCK ) {
      reentrant_store_data_bits( funit->parent, ren, curr_bit );
    }

  }

  PROFILE_END;

}

/*!
 Recursively restores the signal and expression values of the functional units in a reentrant task/function.
*/
static void reentrant_restore_data_bits(
  func_unit*   funit,     /*!< Pointer to current functional unit to restore */
  reentrant*   ren,       /*!< Pointer to reentrant structure containing bits to restore */
  unsigned int curr_bit,  /*!< Current bit in reentrant structure to restore */
  expression*  expr       /*!< Pointer to expression to exclude from updating */
) { PROFILE(REENTRANT_RESTORE_DATA_BITS);

  if( (funit->suppl.part.type == FUNIT_ATASK) || (funit->suppl.part.type == FUNIT_AFUNCTION) || (funit->suppl.part.type == FUNIT_ANAMED_BLOCK) ) {

    unsigned int i;
    unsigned int j;

    /* Walk through each bit in the compressed data array and assign it back to its signal */
    for( j=0; j<funit->sig_size; j++ ) {
      vsignal* sig = funit->sigs[j];
      switch( sig->value->suppl.part.data_type ) {
        case VDATA_UL :
          {
            for( i=0; i<sig->value->width; i++ ) {
              ulong* entry = sig->value->value.ul[UL_DIV(i)];
              if( UL_MOD(i) == 0 ) {
                entry[VTYPE_INDEX_VAL_VALL] = 0;
                entry[VTYPE_INDEX_VAL_VALH] = 0;
              }
              entry[VTYPE_INDEX_VAL_VALL] |= (ulong)((ren->data[curr_bit>>3] >> (curr_bit & 0x7)) & 0x1) << UL_MOD(i);
              curr_bit++;
              entry[VTYPE_INDEX_VAL_VALH] |= (ulong)((ren->data[curr_bit>>3] >> (curr_bit & 0x7)) & 0x1) << UL_MOD(i);
              curr_bit++;
            }
            sig->value->suppl.part.set = (ren->data[curr_bit>>3] >> (curr_bit & 0x7)) & 0x1;
            curr_bit++;
          }
          break;
        case VDATA_R64 :
          {
            uint64 real_bits = 0;
            for( i=0; i<64; i++ ) {
              real_bits |= (uint64)ren->data[curr_bit>>3] << (i - curr_bit);
              curr_bit++;
            }
            sig->value->value.r64->val = sys_task_bitstoreal( real_bits );
          }
          break;
        case VDATA_R32 :
          {
            uint64 real_bits = 0;
            for( i=0; i<32; i++ ) {
              real_bits |= (uint64)ren->data[curr_bit>>3] << (i - curr_bit);
              curr_bit++;
            }
            sig->value->value.r32->val = (float)sys_task_bitstoreal( real_bits );
          }
          break;
        default :  assert( 0 );  break;
      }
    }

    /* Walk through each bit in the compressed data array and assign it back to its expression */
    for( j=0; j<funit->exp_size; j++ ) {
      expression* exp = funit->exps[j];
      if( exp == expr ) {
        curr_bit += (expr->value->width * 2);
      } else {
        if( (EXPR_OWNS_VEC( exp->op ) == 1) && (EXPR_IS_STATIC( exp ) == 0) ) {
          switch( exp->value->suppl.part.data_type ) {
            case VDATA_UL :
              {
                for( i=0; i<exp->value->width; i++ ) {
                  ulong* entry = exp->value->value.ul[UL_DIV(i)];
                  if( UL_MOD(i) == 0 ) {
                    entry[VTYPE_INDEX_VAL_VALL] = 0;
                    entry[VTYPE_INDEX_VAL_VALH] = 0;
                  }
                  entry[VTYPE_INDEX_VAL_VALL] |= (ulong)((ren->data[curr_bit>>3] >> (curr_bit & 0x7)) & 0x1) << UL_MOD(i);
                  curr_bit++;
                  entry[VTYPE_INDEX_VAL_VALH] |= (ulong)((ren->data[curr_bit>>3] >> (curr_bit & 0x7)) & 0x1) << UL_MOD(i);
                  curr_bit++;
                }
              }
              break;
            case VDATA_R64 :
              {
                uint64 real_bits = 0;
                for( i=0; i<64; i++ ) {
                  real_bits |= (uint64)ren->data[curr_bit>>3] << (i - curr_bit);
                  curr_bit++;
                }
                exp->value->value.r64->val = sys_task_bitstoreal( real_bits );
              }
              break;
            case VDATA_R32 :
              {
                uint64 real_bits = 0;
                for( i=0; i<32; i++ ) {
                  real_bits |= (uint64)ren->data[curr_bit>>3] << (i - curr_bit);
                  curr_bit++;
                }
                exp->value->value.r32->val = (float)sys_task_bitstoreal( real_bits );
              }
              break;
            default :  assert( 0 );
          }
        }
      }
      for( i=0; i<(((ESUPPL_BITS_TO_STORE % 2) == 0) ? ESUPPL_BITS_TO_STORE : (ESUPPL_BITS_TO_STORE + 1)); i++ ) {
        switch( i ) {
          case 0 :  exp->suppl.part.left_changed  = (ren->data[curr_bit>>3] >> (curr_bit & 0x7));  break;
          case 1 :  exp->suppl.part.right_changed = (ren->data[curr_bit>>3] >> (curr_bit & 0x7));  break;
          case 2 :  exp->suppl.part.eval_t        = (ren->data[curr_bit>>3] >> (curr_bit & 0x7));  break;
          case 3 :  exp->suppl.part.eval_f        = (ren->data[curr_bit>>3] >> (curr_bit & 0x7));  break;
          case 4 :  exp->suppl.part.prev_called   = (ren->data[curr_bit>>3] >> (curr_bit & 0x7));  break;
        }
        curr_bit++;
      }
    }

    /*
     If the current functional unit is a named block, restore the rest of the bits for the parent functional units
     in this reentrant task/function.
    */
    if( funit->suppl.part.type == FUNIT_ANAMED_BLOCK ) {
      reentrant_restore_data_bits( funit->parent, ren, curr_bit, expr );
    }

  }
 
  PROFILE_END;

}

/*!
 \return Returns a pointer to the newly created reentrant structure.

 Allocates and initializes the reentrant structure for the given functional unit,
 compressing and packing the bits into the given data structure.
*/
reentrant* reentrant_create(
  func_unit* funit  /*!< Pointer to functional unit to create a new reentrant structure for */
) { PROFILE(REENTRANT_CREATE);

  reentrant*   ren  = NULL;  /* Pointer to newly created reentrant structure */
  int          data_size;    /* Number of uint8s needed to store the given functional unit */
  unsigned int bits = 0;     /* Number of bits needed to store signal values */
  int          i;            /* Loop iterator */

  /* Get size needed to store data */
  bits = reentrant_count_afu_bits( funit );

  /* Calculate data size */
  data_size = ((bits & 0x7) == 0) ? (bits >> 3) : ((bits >> 3) + 1);

  /* If there is data to store, allocate the needed memory and populate it */
  if( data_size > 0 ) {

    /* Allocate the structure */
    ren = (reentrant*)malloc_safe( sizeof( reentrant ) );

    /* Set the data size */
    ren->data_size = data_size;

    /* Allocate and initialize memory for data */
    ren->data = (uint8*)malloc_safe( sizeof( uint8 ) * ren->data_size );
    for( i=0; i<data_size; i++ ) {
      ren->data[i] = 0;
    }

    /* Walk through the signal list in the reentrant functional unit, compressing and saving vector values */
    reentrant_store_data_bits( funit, ren, 0 );

  }

  PROFILE_END;

  return( ren );

}

/*!
 Pops data back into the given functional unit and deallocates all memory associated
 with the given reentrant structure.
*/
void reentrant_dealloc(
  reentrant*  ren,    /*!< Pointer to the reentrant structure to deallocate from memory */
  func_unit*  funit,  /*!< Pointer to functional unit associated with this reentrant structure */
  expression* expr    /*!< Pointer of expression to exclude from updating (optional) */
) { PROFILE(REENTRANT_DEALLOC);

  if( ren != NULL ) {

    /* If we have data being stored, pop it */
    if( ren->data_size > 0 ) {

      /* Walk through each bit in the compressed data array and assign it back to its signal */
      reentrant_restore_data_bits( funit, ren, 0, expr );

      /* Deallocate the data uint8 array */
      free_safe( ren->data, (sizeof( uint8 ) * ren->data_size) );

    }

    /* Deallocate memory allocated for this reentrant structure */
    free_safe( ren, sizeof( reentrant ) );

  }

  PROFILE_END;

}

