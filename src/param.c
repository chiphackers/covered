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
 \file     param.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/22/2002
 
 \par
 Providing parameter support for a tool such as Covered was deemed necessary due to its
 influences in signal sizes (affects toggle coverage and expression sizing) and its 
 influences in expression calculation (a parameter can act just like a static value or a
 signal -- depending on how you look at it).  This latter effect can affect the calculation
 of the expression itself which will have a direct effect on combinational logic coverage.
 To accommodate logic designer's usage of the parameter (which can be quite extensive), all
 IEEE1394-1995 compliant parameter-related constructs are supported with the exception of
 defparams (which will be explained later).  In the future, all IEEE1394-2001 parameter
 constructs are planned to be supported.
 
 \par
 Adding parameter support is tricky from the standpoint of making the process of incorporating
 them into the existing Covered structure as easy as possible (changing as little code as
 possible) while still making their handling as efficient as possible.  Additionally tricky
 was the fact that parameters can be used in both expressions and signal declarations.  Since
 parameters can be overridden via defparams (or in Covered's case the -P option -- more on
 this later) or in-line parameter overrides, their values are not the same for each
 instantiation of the module that the parameter is defined in (so the value of the parameter
 must remain with the instance).  However, to keep from having multiple copies of modules
 for each instance (a big efficiency problem in the parsing stage), the expression that makes
 up the value of the parameter needed to stay with the module (instead of copied to all
 instances).
 
 \par
 To accommodate these requirements, two parameter types exist internally in Covered:  module
 parameters and instance parameters.  A module parameter is stored in the module structure and
 contains the expression tree required for calculating the parameter value.  An instance 
 parameter is stored for each parameter in the instance structure.  It contains the value
 of the parameter for the particular instance.  Instance parameter values are always calculated
 immediately upon being added to the instance's instance parameter list.  The two parameter
 structures are linked together via a mod_parm pointer located in the instance parameter
 structure.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>

#include "codegen.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "instance.h"
#include "link.h"
#include "obfuscate.h"
#include "param.h"
#include "static.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


/*!
 This may seem to be odd to store the defparams in a functional unit instance; however, the inst_parm_add
 function has been modified to send in a functional unit instance so we need to pass the defparam head/tail
 in a functional unit instance instead of individual head and tail pointers.  The defparams in this functional
 unit can refer to any functional unit instance, however.
*/
static funit_inst* defparam_list = NULL;

extern char         user_msg[USER_MSG_LENGTH];
extern db**         db_list;
extern unsigned int curr_db;
extern int          curr_sig_id;


/*!
 \return Returns pointer to found module parameter or NULL if module parameter is not
         found.

 Searches specified module parameter list for an module parameter that matches 
 the name of the specified module parameter.  If a match is found, a pointer to 
 the found module parameter is returned to the calling function; otherwise, a value of NULL
 is returned if no match was found.
*/
mod_parm* mod_parm_find(
  const char* name,  /*!< Name of parameter value to find */
  mod_parm*   parm   /*!< Pointer to head of module parameter list to search */
) { PROFILE(MOD_PARM_FIND);

  assert( name != NULL );

  while( (parm != NULL) && ((parm->name == NULL) || (strcmp( parm->name, name ) != 0) || ((parm->suppl.part.type != PARAM_TYPE_DECLARED) && (parm->suppl.part.type != PARAM_TYPE_DECLARED_LOCAL))) ) {
    parm = parm->next;
  }

  PROFILE_END;

  return( parm );
 
}

/*!
 Searches list of module parameter expression lists for specified expression.  If
 the expression is found in one of the lists, remove the expression link.
*/
static void mod_parm_find_expr_and_remove(
  expression* exp,  /*!< Pointer to expression to find and remove from lists */
  mod_parm*   parm  /*!< Pointer to module parameter list to search */
) { PROFILE(MOD_PARM_FIND_EXPR_AND_REMOVE);

  if( exp != NULL ) {

    /* Remove left and right expressions as well */
    mod_parm_find_expr_and_remove( exp->left, parm );
    mod_parm_find_expr_and_remove( exp->right, parm );

    while( parm != NULL ) {
      exp_link_remove( exp, &(parm->exps), &(parm->exp_size), FALSE );
      parm = parm->next;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a string version of the size (width) of the specified signal if it is found; otherwise, return
         a NULL value.
*/
char* mod_parm_gen_size_code(
  vsignal*     sig,        /*!< Specifies the name of the signal that we are searching for */
  unsigned int dimension,  /*!< Specifies the dimension that we are searching for */
  func_unit*   mod,        /*!< Pointer to module containing signal */
  int*         number      /*!< Pointer to value that specifies the returned string as a number (if the values are numbers only) */
) { PROFILE(MOD_PARM_GEN_SIZE_CODE);

  char*        code    = NULL;
  char*        lsb_str = NULL;
  char*        msb_str = NULL;
  unsigned int rv;
  mod_parm*    mparm   = mod->param_head;

  /* First, find the matching LSB module parameter */
  while( (mparm != NULL) && ((mparm->sig != sig) || (mparm->suppl.part.dimension != dimension)) ) {
    mparm = mparm->next;
  }

  if( mparm != NULL ) {

    if( mparm->suppl.part.type == PARAM_TYPE_SIG_LSB ) {
      lsb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
    } else {
      msb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
    }

    /* Second, find the matching MSB module parameter */
    mparm = mparm->next;
    while( (mparm != NULL) && ((mparm->sig != sig) || (mparm->suppl.part.dimension != dimension)) ) {
      mparm = mparm->next;
    }
    if( mparm != NULL ) {
      if( mparm->suppl.part.type == PARAM_TYPE_SIG_LSB ) {
        lsb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
      } else {
        msb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
      }
    }

  }

  if( (lsb_str == NULL) && (msb_str == NULL) ) {

    int msb = sig->dim[dimension].msb;
    int lsb = sig->dim[dimension].lsb;

    *number = (((msb > lsb) ? (msb - lsb) : (lsb - msb)) + 1);

  } else {

    unsigned int slen;

    if( lsb_str == NULL ) {
      char num[50];
      rv = snprintf( num, 50, "%d", sig->dim[dimension].lsb );
      assert( rv < 50 );
      lsb_str = strdup_safe( num );
    }

    if( msb_str == NULL ) {
      char num[50];
      rv = snprintf( num, 50, "%d", sig->dim[dimension].msb );
      assert( rv < 50 );
      msb_str = strdup_safe( num );
    }

    /* Generate code string */
    slen = (strlen( msb_str ) * 3) + (strlen( lsb_str ) * 3) + 30;
    code = (char*)malloc_safe( slen );
    rv   = snprintf( code, slen, "((((%s)>(%s))?((%s)-(%s)):((%s)-(%s)))+1)", msb_str, lsb_str, msb_str, lsb_str, lsb_str, msb_str );
    assert( rv < slen );

    /* Deallocate temporary strings */
    free_safe( lsb_str, (strlen( lsb_str ) + 1) );
    free_safe( msb_str, (strlen( msb_str ) + 1) );

    *number = -1;

  }

  PROFILE_END;

  return( code );

}

/*!
 \return Returns a string version of the LSB of the specified signal if it is found; otherwise, return
         a NULL value.
*/
char* mod_parm_gen_lsb_code(
  vsignal*     sig,        /*!< Specifies the name of the signal that we are searching for */
  unsigned int dimension,  /*!< Specifies the dimension that we are searching for */
  func_unit*   mod,        /*!< Pointer to module containing signal */
  int*         number      /*!< Pointer to value that specifies the returned string as a number (if the values are numbers only) */
) { PROFILE(MOD_PARM_GEN_LSB_CODE);

  char*        code    = NULL;
  char*        lsb_str = NULL;
  char*        msb_str = NULL;
  unsigned int rv;
  mod_parm*    mparm   = mod->param_head;

  *number = -1;

  /* First, find the matching LSB module parameter */
  while( (mparm != NULL) && ((mparm->sig != sig) || (mparm->suppl.part.dimension != dimension)) ) {
    mparm = mparm->next;
  }

  if( mparm != NULL ) {

    if( mparm->suppl.part.type == PARAM_TYPE_SIG_LSB ) {
      lsb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
    } else {
      msb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
    }

    /* Second, find the matching MSB module parameter */
    mparm = mparm->next;
    while( (mparm != NULL) && ((mparm->sig != sig) || (mparm->suppl.part.dimension != dimension)) ) {
      mparm = mparm->next;
    }
    if( mparm != NULL ) {
      if( mparm->suppl.part.type == PARAM_TYPE_SIG_LSB ) {
        lsb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
      } else {
        msb_str = codegen_gen_expr_one_line( mparm->expr, mod, FALSE );
      }
    }

  }

  if( (lsb_str == NULL) && (msb_str == NULL) ) {

    int msb = sig->dim[dimension].msb;
    int lsb = sig->dim[dimension].lsb;

    *number = ((msb > lsb) ? lsb : msb);

  } else {

    unsigned int slen;

    if( lsb_str == NULL ) {
      char num[50];
      rv = snprintf( num, 50, "%d", sig->dim[dimension].lsb );
      assert( rv < 50 );
      lsb_str = strdup_safe( num );
    }

    if( msb_str == NULL ) {
      char num[50];
      rv = snprintf( num, 50, "%d", sig->dim[dimension].msb );
      assert( rv < 50 );
      msb_str = strdup_safe( num );
    }

    /* Generate code string */
    slen = (strlen( msb_str ) * 3) + (strlen( lsb_str ) * 3) + 30;
    code = (char*)malloc_safe( slen );
    rv   = snprintf( code, slen, "(((%s)>(%s))?(%s):(%s))", msb_str, lsb_str, lsb_str, msb_str );
    assert( rv < slen );

    /* Deallocate temporary strings */
    free_safe( lsb_str, (strlen( lsb_str ) + 1) );
    free_safe( msb_str, (strlen( msb_str ) + 1) );

  }

  PROFILE_END;

  return( code );

}

/*!
 \return Returns pointer to newly created module parameter.

 Creates a new module parameter with the specified information and adds 
 it to the module parameter list.
*/
mod_parm* mod_parm_add(
  char*        scope,      /*!< Full hierarchical name of parameter value */
  static_expr* msb,        /*!< Static expression containing the MSB of this module parameter */
  static_expr* lsb,        /*!< Static expression containing the LSB of this module parameter */
  bool         is_signed,  /*!< Specifies if this parameter needs to be handled as a signed value */
  expression*  expr,       /*!< Expression tree for current module parameter */
  int          type,       /*!< Specifies type of module parameter (declared/override) */
  func_unit*   funit,      /*!< Functional unit to add this module parameter to */
  char*        inst_name   /*!< Name of instance (used for parameter overridding) */
) { PROFILE(MOD_PARM_ADD);

  mod_parm*  parm;       /* Temporary pointer to instance parameter */
  mod_parm*  curr;       /* Pointer to current module parameter for ordering purposes */
  int        order = 0;  /* Current order of parameter */
  func_unit* mod_funit;  /* Pointer to module containing this functional unit (used for ordering purposes) */
  
  assert( (type == PARAM_TYPE_OVERRIDE) || (expr != NULL) );  /* An expression can be NULL if we are an override type */
  assert( (type == PARAM_TYPE_DECLARED)       || 
          (type == PARAM_TYPE_DECLARED_LOCAL) ||
          (type == PARAM_TYPE_OVERRIDE)       ||
          (type == PARAM_TYPE_SIG_LSB)        ||
          (type == PARAM_TYPE_SIG_MSB)        ||
          (type == PARAM_TYPE_INST_LSB)       ||
          (type == PARAM_TYPE_INST_MSB) );

  /* Find module containing this functional unit */
  mod_funit = funit_get_curr_module( funit );

  /* Determine parameter order */
  if( type == PARAM_TYPE_DECLARED ) {
    curr  = mod_funit->param_head;
    order = 0;
    while( curr != NULL ) {
      if( curr->suppl.part.type == PARAM_TYPE_DECLARED ) {
        order++;
      }
      curr = curr->next;
    }
  } else if( type == PARAM_TYPE_OVERRIDE ) {
    curr  = mod_funit->param_head;
    order = 0;
    while( curr != NULL ) {
      if( (curr->suppl.part.type == PARAM_TYPE_OVERRIDE) &&
          (strcmp( inst_name, curr->inst_name ) == 0) ) {
        order++;
      }
      curr = curr->next;
    }
  }

  /* Create new signal/expression binding */
  parm = (mod_parm*)malloc_safe( sizeof( mod_parm ) );
  if( scope != NULL ) {
    parm->name = strdup_safe( scope );
  } else {
    parm->name = NULL;
  }
  if( inst_name != NULL ) {
    parm->inst_name = strdup_safe( inst_name );
  } else {
    parm->inst_name = NULL;
  }
  if( msb != NULL ) {
    parm->msb      = (static_expr*)malloc_safe( sizeof( static_expr ) );
    parm->msb->num = msb->num;
    parm->msb->exp = msb->exp;
  } else {
    parm->msb = NULL;
  }
  if( lsb != NULL ) {
    parm->lsb      = (static_expr*)malloc_safe( sizeof( static_expr ) );
    parm->lsb->num = lsb->num;
    parm->lsb->exp = lsb->exp;
  } else {
    parm->lsb = NULL;
  }
  parm->is_signed             = is_signed;
  parm->expr                  = expr;
  parm->suppl.all             = 0;
  parm->suppl.part.type       = type;
  parm->suppl.part.order      = order;
  if( expr != NULL ) {
    if( expr->suppl.part.owned == 0 ) {
      parm->suppl.part.owns_expr = 1;
      expr->suppl.part.owned = 1;
    }
  }
  parm->exps                  = NULL;
  parm->exp_size              = 0;
  parm->sig                   = NULL;
  parm->next                  = NULL;

  /* Now add the parameter to the current expression */
  if( funit->param_head == NULL ) {
    funit->param_head = funit->param_tail = parm;
  } else {
    funit->param_tail->next = parm;
    funit->param_tail       = parm;
  }

  PROFILE_END;

  return( parm );

}

/*!
 \return Returns a value of 1 so that this may be called within an expression, if necessary.

 Outputs contents of specified module parameter to standard output.
 For debugging purposes only.
*/
int mod_parm_display(
  mod_parm* mparm  /*!< Pointer to module parameter list to display */
) {

  char type_str[30];  /* String version of module parameter type */

  while( mparm != NULL ) {
    switch( mparm->suppl.part.type ) {
      case PARAM_TYPE_DECLARED       :  strcpy( type_str, "DECLARED" );        break;
      case PARAM_TYPE_OVERRIDE       :  strcpy( type_str, "OVERRIDE" );        break;
      case PARAM_TYPE_SIG_LSB        :  strcpy( type_str, "SIG_LSB"  );        break;
      case PARAM_TYPE_SIG_MSB        :  strcpy( type_str, "SIG_MSB"  );        break;
      case PARAM_TYPE_INST_LSB       :  strcpy( type_str, "INST_LSB" );        break;
      case PARAM_TYPE_INST_MSB       :  strcpy( type_str, "INST_MSB" );        break;
      case PARAM_TYPE_DECLARED_LOCAL :  strcpy( type_str, "DECLARED_LOCAL" );  break;
      default                        :  strcpy( type_str, "UNKNOWN" );         break;
    }
    if( mparm->name == NULL ) {
      printf( "  mparam => type: %s, order: %u, owns_exp: %u",
              type_str, mparm->suppl.part.order, mparm->suppl.part.owns_expr );
    } else {
      printf( "  mparam => name: %s, type: %s, order: %u, owns_exp: %u",
               obf_sig( mparm->name ), type_str, mparm->suppl.part.order, mparm->suppl.part.owns_expr );
    }
    if( mparm->expr != NULL ) {
      printf( ", exp_id: %d\n", mparm->expr->id );
    } else {
      printf( ", no_expr\n" );
    }
    if( mparm->sig != NULL ) {
      printf( "    " );  vsignal_display( mparm->sig );
    }
    printf( "    " );  exp_link_display( mparm->exps, mparm->exp_size );
    mparm = mparm->next;
  }

  return( 1 );

}

/*******************************************************************************/

/*!
 \return Returns pointer to found instance parameter or NULL if instance parameter is not
         found.

 Searches specified instance parameter list for an instance parameter that matches 
 the name of the specified instance parameter.  If a match is found, a pointer to 
 the found instance parameter is returned to the calling function; otherwise, a value of NULL
 is returned if no match was found.
*/
static inst_parm* inst_parm_find(
  const char* name,  /*!< Name of parameter value to find */
  inst_parm*  iparm  /*!< Pointer to head of instance parameter list to search */
) { PROFILE(INST_PARM_FIND);

  assert( name != NULL );

  while( (iparm != NULL) && ((iparm->sig == NULL) || (iparm->sig->name == NULL) || (strcmp( iparm->sig->name, name ) != 0)) ) {
    iparm = iparm->next;
  }

  PROFILE_END;

  return( iparm );
 
}

/*!
 \return Returns pointer to newly created instance parameter.

 \throws anonymous Throw param_expr_eval param_expr_eval expression_set_value

 Creates a new instance parameter with the specified information and adds 
 it to the instance parameter list.
*/
static inst_parm* inst_parm_add(
  const char*  name,       /*!< Name of parameter */
  char*        inst_name,  /*!< Name of instance containing this parameter name */
  static_expr* msb,        /*!< Static expression containing the MSB of this instance parameter */
  static_expr* lsb,        /*!< Static expression containing the LSB of this instance parameter */
  bool         is_signed,  /*!< Specifies if this instance parameter should be treated as signed or unsigned */
  vector*      value,      /*!< Vector value of specified instance parameter */
  mod_parm*    mparm,      /*!< Pointer to module instance that this instance parameter is derived from */
  funit_inst*  inst        /*!< Pointer to current functional unit instance */
) { PROFILE(INST_PARM_ADD);

  inst_parm* iparm     = NULL;  /* Temporary pointer to instance parameter */
  int        sig_width;         /* Width of this parameter signal */
  int        sig_lsb;           /* LSB of this parameter signal */
  int        sig_be;            /* Big endianness of this parameter signal */
  int        sig_type;          /* Type of signal parameter to create */
  int        left_val  = 31;    /* Value of left (msb) static expression */
  int        right_val = 0;     /* Value of right (lsb) static expression */
  
  assert( value != NULL );
  assert( ((msb == NULL) && (lsb == NULL)) || ((msb != NULL) && (lsb != NULL)) );

  /* Only add the instance parameter if it currently does not exist */
  if( (name == NULL) || (inst_name != NULL) || (inst_parm_find( name, inst->param_head ) == NULL) ) {

    /* Create new signal/expression binding */
    iparm = (inst_parm*)malloc_safe( sizeof( inst_parm ) );

    if( inst_name != NULL ) {
      iparm->inst_name = strdup_safe( inst_name );
    } else {
      iparm->inst_name = NULL;
    }

    Try {

      /* If the MSB/LSB was specified, calculate the LSB and width values */
      if( msb != NULL ) {

        /* Calculate left value */
        if( lsb->exp != NULL ) {
          param_expr_eval( lsb->exp, inst );
          right_val = vector_to_int( lsb->exp->value );
        } else {
          right_val = lsb->num;
        }
        assert( right_val >= 0 );

        /* Calculate right value */
        if( msb->exp != NULL ) {
          param_expr_eval( msb->exp, inst );
          left_val = vector_to_int( msb->exp->value );
        } else {
          left_val = msb->num;
        }
        assert( left_val >= 0 );

        /* Calculate LSB and width information */
        if( right_val > left_val ) {
          sig_lsb   = left_val;
          sig_width = (right_val - left_val) + 1;
          sig_be    = 1;
        } else {
          sig_lsb   = right_val;
          sig_width = (left_val - right_val) + 1;
          sig_be    = 0;
        }

      } else {

        sig_lsb   = 0;
        sig_width = value->width;
        sig_be    = 0;

      }

      /* If the parameter is sized too big, panic */
      assert( (sig_width <= MAX_BIT_WIDTH) && (sig_width >= 0) );

      /* Figure out what type of parameter this signal needs to be */
      if( (value != NULL) && ((value->suppl.part.data_type == VDATA_R64) || (value->suppl.part.data_type == VDATA_R32)) ) {
        sig_type = SSUPPL_TYPE_PARAM_REAL;
      } else {
        sig_type = SSUPPL_TYPE_PARAM;
      }

      /* Create instance parameter signal */
      iparm->sig = vsignal_create( name, sig_type, sig_width, 0, 0 );
      iparm->sig->pdim_num   = 1;
      iparm->sig->dim        = (dim_range*)malloc_safe( sizeof( dim_range ) * 1 );
      iparm->sig->dim[0].lsb = right_val;
      iparm->sig->dim[0].msb = left_val;
      iparm->sig->suppl.part.big_endian = sig_be;

      /* Store signed attribute for this vector */
      iparm->sig->value->suppl.part.is_signed = is_signed;
  
      /* Copy the contents of the specified vector value to the signal */
      switch( value->suppl.part.data_type ) {
        case VDATA_UL :
          (void)vector_set_value_ulong( iparm->sig->value, value->value.ul, value->width );
          break;
        case VDATA_R64 :
          (void)vector_from_real64( iparm->sig->value, value->value.r64->val );
          break;
        case VDATA_R32 :
          (void)vector_from_real64( iparm->sig->value, (double)value->value.r32->val );
          break;
        default :  assert( 0 );  break;
      }

      iparm->mparm = mparm;
      iparm->next  = NULL;

      /* Bind the module parameter expression list to this signal */
      if( mparm != NULL ) {
        unsigned int i;
        for( i=0; i<mparm->exp_size; i++ ) {
          mparm->exps[i]->sig = iparm->sig;
          /* Set the expression's vector to this signal's vector if we are part of a generate expression */
          if( mparm->exps[i]->suppl.part.gen_expr == 1 ) {
            expression_set_value( mparm->exps[i], iparm->sig, inst->funit );
          }
          exp_link_add( mparm->exps[i], &(iparm->sig->exps), &(iparm->sig->exp_size) );
        }
      }

      /* Now add the parameter to the current expression */
      if( inst->param_head == NULL ) {
        inst->param_head = inst->param_tail = iparm;
      } else {
        inst->param_tail->next = iparm;
        inst->param_tail       = iparm;
      }
  
    } Catch_anonymous {
      inst_parm_dealloc( iparm, FALSE );
      Throw 0;
    }

  }

  PROFILE_END;

  return( iparm );

}

/*!
 Creates an instance parameter for a generate variable and adds it to the
 given instance parameter list.
*/
void inst_parm_add_genvar(
  vsignal*    sig,  /*!< Pointer to generate signal to copy */
  funit_inst* inst  /*!< Pointer to instance to add this instance parameter to */
) { PROFILE(INST_PARM_ADD_GENVAR);

  inst_parm* iparm;  /* Pointer to the newly allocated instance parameter */

  assert( inst != NULL );

  /* Allocate the new instance parameter */
  iparm = (inst_parm*)malloc_safe( sizeof( inst_parm ) );

  /* Initialize the instance parameter */
  iparm->inst_name            = NULL;
  iparm->sig                  = vsignal_duplicate( sig ); 
  iparm->sig->suppl.part.type = SSUPPL_TYPE_PARAM;
  iparm->mparm                = NULL;
  iparm->next                 = NULL;

  /* Add the instance parameter to the parameter list */
  if( inst->param_head == NULL ) {
    inst->param_head = inst->param_tail = iparm;
  } else {
    inst->param_tail->next = iparm;
    inst->param_tail       = iparm;
  }

  PROFILE_END;

}

/*!
 Binds the instance parameter signal to its list of expressions.  This is called
 by funit_size_elements.
*/
void inst_parm_bind(
  inst_parm* iparm  /*!< Pointer to instance parameter to bind */
) { PROFILE(INST_PARM_BIND);

  /* Bind the module parameter expression list to this signal */
  if( iparm->mparm != NULL ) {
    unsigned int i;
    for( i=0; i<iparm->mparm->exp_size; i++ ) {
      iparm->mparm->exps[i]->sig = iparm->sig;
    }
  }

  PROFILE_END;

}


/************************************************************************************/

/*!
 \throws anonymous inst_parm_add Throw

 Scans list of all parameters to make sure that specified parameter isn't already
 being set to a new value.  If no match occurs, adds the new defparam to the
 defparam list.  This function is called for each -P option to the score command.
*/
void defparam_add(
  const char* scope,  /*!< Full hierarchical reference to specified scope to change value to */
  vector*     value   /*!< User-specified parameter override value */
) { PROFILE(DEFPARAM_ADD);

  static_expr msb;  /* MSB of this defparam (forced to be 31) */
  static_expr lsb;  /* LSB of this defparam (forced to be 0) */

  assert( scope != NULL );

  /* If the defparam instance doesn't exist, create it now */
  if( defparam_list == NULL ) {
    defparam_list = (funit_inst*)malloc_safe( sizeof( funit_inst ) );
    defparam_list->param_head = NULL;
    defparam_list->param_tail = NULL;
  }

  if( inst_parm_find( scope, defparam_list->param_head ) == NULL ) {

    /* Generate MSB and LSB information */
    switch( value->suppl.part.data_type ) {
      case VDATA_UL  :  msb.num = 31;  break;
      case VDATA_R64 :  msb.num = 63;  break;
      case VDATA_R32 :  msb.num = 31;  break;
      default        :  assert( 0 );   break;
    }
    msb.exp = NULL;
    lsb.num = 0;
    lsb.exp = NULL;

    Try {
      (void)inst_parm_add( scope, NULL, &msb, &lsb, FALSE, value, NULL, defparam_list );
    } Catch_anonymous {
      vector_dealloc( value );
      Throw 0;
    }

    vector_dealloc( value );

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Parameter (%s) value is assigned more than once", obf_sig( scope ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Deallocates all memory used for storing defparam information.
*/
void defparam_dealloc() { PROFILE(DEFPARAM_DEALLOC);

  if( defparam_list != NULL ) {

    /* Deallocate the instance parameters in the defparam_list structure */
    inst_parm_dealloc( defparam_list->param_head, TRUE );

    /* Now free the defparam_list structure itself */
    free_safe( defparam_list, sizeof( funit_inst ) );

  }

  PROFILE_END;

}

/*************************************************************************************/

/*!
 \return Returns a pointer to the specified value found.

 \throws anonymous Throw expression_set_value param_find_and_set_expr_value

 This function is called by param_expr_eval when it encounters a parameter in its
 expression tree that needs to be resolved for its value.  If the parameter is
 found, the value of that parameter is returned.  If the parameter is not found,
 an error message is displayed to the user (the user has created a module in which
 a parameter value is used without being defined).
*/
static void param_find_and_set_expr_value(
  expression* expr,  /*!< Pointer to current expression to evaluate */
  funit_inst* inst   /*!< Pointer to current instance to search */
) { PROFILE(PARAM_FIND_AND_SET_EXPR_VALUE);

  inst_parm* icurr;  /* Pointer to current instance parameter being evaluated */
    
  if( inst != NULL ) {

    icurr = inst->param_head;
    while( (icurr != NULL) && ((icurr->mparm == NULL) || (exp_link_find( expr->id, icurr->mparm->exps, icurr->mparm->exp_size ) == NULL)) ) {
      icurr = icurr->next;
    }

    /*
     If we were unable to find the module parameter in the current instance, check the rest of our
     scope for the value.
    */
    if( icurr == NULL ) {

      if( inst->funit->parent != NULL ) {
        param_find_and_set_expr_value( expr, inst->parent );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Parameter used in expression but not defined in current module, file: %s, line %u", obf_file( inst->funit->orig_fname ), expr->line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

    } else {

      /* Set the found instance parameter value to this expression */
      expression_set_value( expr, icurr->sig, inst->funit );

      /* Cause expression/signal to point to each other */
      expr->sig = icurr->sig;
      
      exp_link_add( expr, &(icurr->sig->exps), &(icurr->sig->exp_size) );

    }

  }

  PROFILE_END;
  
}

/*!
 Sizes the specified signal according to the value of the specified
 instance parameter value.
*/
void param_set_sig_size(
  vsignal*   sig,   /*!< Pointer to signal to search for in instance parameter list */
  inst_parm* icurr  /*!< Pointer to head of instance parameter list to search */
) { PROFILE(PARAM_SET_SIG_SIZE);

  assert( sig != NULL );
  assert( icurr != NULL );
  assert( icurr->sig != NULL );
  assert( icurr->mparm != NULL );

  /* Set the LSB/MSB to the value of the given instance parameter */
  if( icurr->mparm->suppl.part.type == PARAM_TYPE_SIG_LSB ) {
    sig->dim[icurr->mparm->suppl.part.dimension].lsb = vector_to_int( icurr->sig->value );
  } else {
    sig->dim[icurr->mparm->suppl.part.dimension].msb = vector_to_int( icurr->sig->value );
  }

  PROFILE_END;

}

/*************************************************************************************/

/*!
 \throws anonymous funit_size_elements param_size_function param_resolve

 Recursively iterates through all functional units of given function, sizing them as
 appropriate for the purposes of static function allocation and execution.
*/
static void param_size_function(
  funit_inst* inst,  /*!< Pointer to instance pointing to given functional unit */
  func_unit*  funit  /*!< Pointer to functional unit to size */
) { PROFILE(PARAM_SIZE_FUNCTION);

  funit_inst* child;  /* Pointer to current child instance */

  /* Resolve all parameters for this instance */
  param_resolve( inst );

  /* Resize the current functional unit */
  funit_size_elements( funit, inst, FALSE, TRUE );

  /* Recursively iterate through list of children instances */
  child = inst->child_head;
  while( child != NULL ) {
    param_size_function( child, child->funit );
    child = child->next;
  }

  PROFILE_END;

}

/*!
 \throws anonymous expression_resize param_expr_eval param_expr_eval param_size_function param_find_and_set_expr_value

 Recursively evaluates the specified expression tree, calculating the value of leaf nodes
 first.  If a another parameter value is encountered, lookup the value of this parameter
 in the current instance instance parameter list.  If the instance parameter cannot be
 found, we have encountered a user error; therefore, display an error message to the
 user indicating such.
*/
void param_expr_eval(
  expression* expr,  /*!< Current expression to evaluate */
  funit_inst* inst   /*!< Pointer to current instance to evaluate for */
) { PROFILE(PARAM_EXPR_EVAL);

  funit_inst* funiti;      /* Pointer to static function instance */
  func_unit*  funit;       /* Pointer to constant function */
  int         ignore = 0;  /* Number of instances to ignore */

  if( expr != NULL ) {

    /* Initialize the current time */
    sim_time time = {0,0,0,FALSE};

    /* For constant functions, resolve parameters and resize the functional unit first */
    if( expr->op == EXP_OP_FUNC_CALL ) {
      funit = expr->elem.funit;
      assert( funit != NULL );
      funiti = instance_find_by_funit( inst, funit, &ignore );
      assert( funiti != NULL );
      param_size_function( funiti, funit );
    }

    /* Evaluate children first */
    param_expr_eval( expr->left,  inst );
    param_expr_eval( expr->right, inst );

    switch( expr->op ) {
      case EXP_OP_STATIC  :
      case EXP_OP_PASSIGN :
        break;
      case EXP_OP_PARAM          :
      case EXP_OP_PARAM_SBIT     :
      case EXP_OP_PARAM_MBIT     :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        param_find_and_set_expr_value( expr, inst );
        break;
      case EXP_OP_SIG :
        assert( expr->sig != NULL );
        assert( expr->sig->suppl.part.type == SSUPPL_TYPE_GENVAR );
        break;
      default :
        /*
         Since we are not a parameter identifier, let's allocate some data for us 
         if we don't have some already.
        */
        assert( expr->value != NULL );
        assert( (expr->op != EXP_OP_SBIT_SEL) &&
                (expr->op != EXP_OP_MBIT_SEL) &&
                (expr->op != EXP_OP_MBIT_POS) &&
                (expr->op != EXP_OP_MBIT_NEG) );
        expression_resize( expr, inst->funit, FALSE, TRUE );
        break;
    }

    /* Perform the operation */
    (void)expression_operate( expr, NULL, &time );

  }

  PROFILE_END;
  
}

/*************************************************************************************/

/*!
 \return Returns a pointer to the newly created instance parameter or NULL if one is not created

 \throws anonymous inst_parm_add

 Looks up in the parent instance instance parameter list for overrides.  If an
 override is found, adds the new instance parameter using the value of the
 override.  If no override is found, returns NULL and does nothing.
*/
static inst_parm* param_has_override(
  mod_parm*   mparm,  /*!< Pointer to parameter in current module to check */
  funit_inst* inst    /*!< Pointer to current instance */
) { PROFILE(PARAM_HAS_OVERRIDE);

  inst_parm*  icurr = NULL;  /* Pointer to current instance parameter in parent */
  inst_parm*  parm  = NULL;  /* Pointer to newly created parameter (if one is created) */
  funit_inst* mod_inst;      /* Pointer to the instance that refers to the module containing this instance */

  assert( mparm != NULL );
  assert( inst != NULL );

  /* Find the module instance for this instance */
  mod_inst = inst;
  while( mod_inst->funit->parent != NULL ) {
    mod_inst = mod_inst->parent;
  }

  /* Check to see if the parent instance contains an override in its instance list. */
  if( mod_inst->parent != NULL ) {

    icurr = mod_inst->parent->param_head;
    while( (icurr != NULL) && 
           ((icurr->mparm == NULL) ||
            !((icurr->mparm->suppl.part.type == PARAM_TYPE_OVERRIDE) &&
              (mparm->suppl.part.type != PARAM_TYPE_DECLARED_LOCAL) &&
              ((icurr->sig->name != NULL) ? (strcmp( icurr->sig->name, mparm->name ) == 0) :
                                            (mparm->suppl.part.order == icurr->mparm->suppl.part.order )) &&
              (strcmp( mod_inst->name, icurr->inst_name ) == 0))) ) {
      icurr = icurr->next;
    }

  }

  /* If an override has been found, use this value instead of the mparm expression value */
  if( icurr != NULL ) {

    /* Add new instance parameter to current instance */
    parm = inst_parm_add( mparm->name, NULL, mparm->msb, mparm->lsb, mparm->is_signed, icurr->sig->value, mparm, inst );

  }

  PROFILE_END;

  return( parm );

}

/*!
 \return Returns pointer to created instance parameter or NULL if one is not created.

 \throws anonymous inst_parm_add

 Looks up specified parameter in defparam list.  If a match is found, a new
 instance parameter is created with the value of the found defparam.  If a match
 is not found, return NULL and do nothing else.
*/
static inst_parm* param_has_defparam(
  mod_parm*   mparm,  /*!< Pointer to module parameter to attach new instance parameter to */
  funit_inst* inst    /*!< Pointer to current instance */
) { PROFILE(PARAM_HAS_DEFPARAM);

  inst_parm* parm = NULL;       /* Pointer newly created instance parameter (if one is created) */
  inst_parm* icurr;             /* Pointer to current defparam */
  char       parm_scope[4096];  /* Specifes full scope to parameter to find */
  char       scope[4096];       /* Scope of this instance */

  assert( mparm != NULL );
  assert( inst != NULL );

  /* Make sure that the user specified at least one defparam */
  if( defparam_list != NULL ) {

    unsigned int rv;

    /* Get scope of this instance */
    scope[0] = '\0';
    instance_gen_scope( scope, inst, FALSE );

    assert( db_list[curr_db]->leading_hier_num > 0 );

    /* Generate full hierarchy of this parameter */
    rv = snprintf( parm_scope, 4096, "%s.%s", scope, mparm->name );
    assert( rv < 4096 );

    icurr = defparam_list->param_head;
    while( (icurr != NULL) &&
           !((strcmp( icurr->sig->name, parm_scope ) == 0) &&
             (mparm->suppl.part.type != PARAM_TYPE_DECLARED_LOCAL)) ) {
      icurr = icurr->next;
    }

    if( icurr != NULL ) {

      /* Defparam found, use its value to create new instance parameter */
      parm = inst_parm_add( mparm->name, NULL, mparm->msb, mparm->lsb, mparm->is_signed, icurr->sig->value, mparm, inst );

    }

  }

  PROFILE_END;

  return( parm );

}

/*!
 \throws anonymous inst_parm_add param_expr_eval

 Performs declared module parameter resolution and stores the appropriate
 instance parameter into the current instance's instance parameter list.  This
 procedure is accomplished by checking the following in the specified order:
 -# Check to see if parameter has an override parameter by checking the
    instance parameter list of the parent instance to this instance.
 -# If (1) fails, check to see if current parameter is overridden by a user-specified
    defparam value.
 -# If (2) fails, calculate the current expression's value by evaluating the
    parameter's expression tree.
*/
static void param_resolve_declared(
  mod_parm*   mparm,  /*!< Pointer to parameter in current module to check */
  funit_inst* inst    /*!< Pointer to current instance */
) { PROFILE(PARAM_RESOLVE_DECLARED);

  assert( mparm != NULL );

  if( param_has_override( mparm, inst ) != NULL ) {

    /* Parameter override was found in parent module, do nothing more */

  } else if( param_has_defparam( mparm, inst ) != NULL ) {

    /* Parameter defparam override was found, do nothing more */

  } else {
    
    assert( mparm->expr != NULL );

    /* First evaluate the current module expression */
    param_expr_eval( mparm->expr, inst );

    /* Now add the new instance parameter */
    (void)inst_parm_add( mparm->name, NULL, mparm->msb, mparm->lsb, mparm->is_signed, mparm->expr->value, mparm, inst );

  }

  PROFILE_END;

}

/************************************************************************************/

/*!
 \throws anonymous inst_parm_add param_expr_eval

 Evaluates the current module parameter expression tree and adds a new instance
 parameter to the specified instance parameter list, preserving the order and
 type of the override parameter.
*/
static void param_resolve_override(
  mod_parm*   oparm,  /*!< Pointer to override module parameter */
  funit_inst* inst    /*!< Pointer to instance to add new instance parameter to */
) { PROFILE(PARAM_RESOLVE_OVERRIDE);

  assert( oparm != NULL );

  /* If this is a NULL parameter, don't attempt an expression evaluation */
  if( oparm->expr != NULL ) {

    /* Evaluate module override parameter */
    param_expr_eval( oparm->expr, inst );

    /* Add the new instance override parameter */
    (void)inst_parm_add( oparm->name, oparm->inst_name, oparm->msb, oparm->lsb, oparm->is_signed, oparm->expr->value, oparm, inst );

  }

  PROFILE_END;

}

/*!
 Called after local binding has occurred.  Resolves the parameters for the given functional unit
 instance.
*/
void param_resolve_inst(
  funit_inst* inst  /*!< Pointer to functional unit instance to resolve parameter values for */
) { PROFILE(PARAM_RESOLVE_INST);

  assert( inst != NULL );

  /* Resolve this instance */
  if( inst->funit != NULL ) {
    mod_parm* mparm = inst->funit->param_head;
    while( mparm != NULL ) {
      if( (mparm->suppl.part.type == PARAM_TYPE_DECLARED) ||
          (mparm->suppl.part.type == PARAM_TYPE_DECLARED_LOCAL) ) {
        param_resolve_declared( mparm, inst );
      } else {
        param_resolve_override( mparm, inst );
      }
      mparm = mparm->next;
    }
  }

  PROFILE_END;

}

/*!
 \throws anonymous param_resolve_override param_resolve param_resolve_declared

 Called after binding has occurred.  Recursively resolves all parameters for the given
 instance tree.
*/
void param_resolve(
  funit_inst* inst  /*!< Pointer to functional unit instance to resolve parameter values for */
) { PROFILE(PARAM_RESOLVE);

  funit_inst* child;  /* Pointer to child instance of this instance */

  assert( inst != NULL );

  /* Resolve this instance */
  param_resolve_inst( inst );

  /* Resolve all child instances */
  child = inst->child_head;
  while( child != NULL ) {
    param_resolve( child );
    child = child->next;
  }

  PROFILE_END;

}

/*!
 Prints contents of specified instance parameter to the specified output stream.
 Parameters get output in the same format as signals (they type specified for parameters
 is DB_TYPE_SIGNAL).  A leading # sign is attached to the parameter name to indicate
 that the current signal is a parameter and not a signal, and should therefore not
 be scored as a signal.
*/
void param_db_write(
  inst_parm* iparm,  /*!< Pointer to instance parameter to output to file */
  FILE*      file    /*!< Pointer to file handle to write parameter contents to */
) { PROFILE(PARAM_DB_WRITE);

  /*
   If the parameter does not have a name, it will not be used in expressions;
   therefore, there is no reason to output this parameter to the CDD file.
  */
  if( iparm->sig->name != NULL ) {

    /* Assign a signal ID and increment it for the next signal -- this is okay to do because parameters only exist during parsing */
    iparm->sig->id = curr_sig_id++;

    /* Write the signal */
    vsignal_db_write( iparm->sig, file );

  }

  PROFILE_END;

}

/**********************************************************************************/

/*!
 Deallocates allocated memory from heap for the specified module parameter.  If
 the value of recursive is set to TRUE, perform this deallocation for the entire
 list of module parameters.
*/
void mod_parm_dealloc(
  mod_parm* parm,      /*!< Pointer to module parameter to remove */
  bool      recursive  /*!< If TRUE, removes entire module parameter list; otherwise, just remove me */
) { PROFILE(MOD_PARM_DEALLOC);

  if( parm != NULL ) {

    /* If the user wants to deallocate the entire module parameter list, do so now */
    if( recursive ) {
      mod_parm_dealloc( parm->next, recursive );
    }

    /* Deallocate MSB and LSB static expressions */
    static_expr_dealloc( parm->msb, TRUE );
    static_expr_dealloc( parm->lsb, TRUE );

    /* Remove the attached expression tree */
    if( parm->suppl.part.owns_expr == 1 ) {
      expression_dealloc( parm->expr, FALSE );
    }

    /* Remove the expression list that this parameter is used in */
    exp_link_delete_list( parm->exps, parm->exp_size, FALSE );

    /* Remove the parameter name */
    free_safe( parm->name, (strlen( parm->name ) + 1) );

    /* Remove instance name, if specified */
    free_safe( parm->inst_name, (strlen( parm->inst_name ) + 1) );

    /* Remove the parameter itself */
    free_safe( parm, sizeof( mod_parm ) );

  }

  PROFILE_END;

}

/*!
 Deallocates allocated memory from heap for the specified instance parameter.  If
 the value of recursive is set to TRUE, perform this deallocation for the entire
 list of instance parameters.
*/
void inst_parm_dealloc(
  inst_parm* iparm,     /*!< Pointer to instance parameter to remove */
  bool       recursive  /*!< If TRUE, removes entire instance parameter list; otherwise, just remove me */
) { PROFILE(INST_PARM_DEALLOC);

  if( iparm != NULL ) {

    /* If the user wants to deallocate the entire module parameter list, do so now */
    if( recursive ) {
      inst_parm_dealloc( iparm->next, recursive );
    }

    /* Deallocate parameter signal */
    vsignal_dealloc( iparm->sig );

    /* Deallocate instance name, if specified */
    if( iparm->inst_name != NULL ) {
      free_safe( iparm->inst_name, (strlen( iparm->inst_name ) + 1) );
    }
    
    /* Deallocate parameter itself */
    free_safe( iparm, sizeof( inst_parm ) );

  }

  PROFILE_END;

}

