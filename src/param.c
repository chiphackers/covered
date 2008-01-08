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
funit_inst*   defparam_list = NULL;

extern char   user_msg[USER_MSG_LENGTH];
extern char** leading_hierarchies;
extern int    leading_hier_num;


/*!
 \param name  Name of parameter value to find.
 \param parm  Pointer to head of module parameter list to search.

 \return Returns pointer to found module parameter or NULL if module parameter is not
         found.

 Searches specified module parameter list for an module parameter that matches 
 the name of the specified module parameter.  If a match is found, a pointer to 
 the found module parameter is returned to the calling function; otherwise, a value of NULL
 is returned if no match was found.
*/
mod_parm* mod_parm_find( char* name, mod_parm* parm ) { PROFILE(MOD_PARM_FIND);

  assert( name != NULL );

  while( (parm != NULL) && ((parm->name == NULL) || (strcmp( parm->name, name ) != 0)) ) {
    parm = parm->next;
  }

  return( parm );
 
}

/*!
 \param exp   Pointer to expression to find and remove from lists.
 \param parm  Pointer to module parameter list to search.

 Searches list of module parameter expression lists for specified expression.  If
 the expression is found in one of the lists, remove the expression link.
*/
void mod_parm_find_expr_and_remove( expression* exp, mod_parm* parm ) { PROFILE(MOD_PARM_FIND_EXPR_AND_REMOVE);

  if( exp != NULL ) {

    /* Remove left and right expressions as well */
    mod_parm_find_expr_and_remove( exp->left, parm );
    mod_parm_find_expr_and_remove( exp->right, parm );

    while( parm != NULL ) {
      exp_link_remove( exp, &(parm->exp_head), &(parm->exp_tail), FALSE );
      parm = parm->next;
    }

  }

}

/*!
 \param scope      Full hierarchical name of parameter value.
 \param msb        Static expression containing the MSB of this module parameter.
 \param lsb        Static expression containing the LSB of this module parameter.
 \param is_signed  Specifies if this parameter needs to be handled as a signed value.
 \param expr       Expression tree for current module parameter.
 \param type       Specifies type of module parameter (declared/override).
 \param funit      Functional unit to add this module parameter to.
 \param inst_name  Name of instance (used for parameter overridding)

 \return Returns pointer to newly created module parameter.

 Creates a new module parameter with the specified information and adds 
 it to the module parameter list.
*/
mod_parm* mod_parm_add( char* scope, static_expr* msb, static_expr* lsb, bool is_signed,
                        expression* expr, int type, func_unit* funit, char* inst_name ) { PROFILE(MOD_PARM_ADD);

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
  parm->exp_head              = NULL;
  parm->exp_tail              = NULL;
  parm->sig                   = NULL;
  parm->next                  = NULL;

  /* Now add the parameter to the current expression */
  if( funit->param_head == NULL ) {
    funit->param_head = funit->param_tail = parm;
  } else {
    funit->param_tail->next = parm;
    funit->param_tail       = parm;
  }

  return( parm );

}

/*!
 \param mparm  Pointer to module parameter list to display.

 Outputs contents of specified module parameter to standard output.
 For debugging purposes only.
*/
void mod_parm_display( mod_parm* mparm ) {

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
    printf( "    " );  exp_link_display( mparm->exp_head );
    mparm = mparm->next;
  }

}

/*******************************************************************************/

/*!
 \param name   Name of parameter value to find.
 \param iparm  Pointer to head of instance parameter list to search.

 \return Returns pointer to found instance parameter or NULL if instance parameter is not
         found.

 Searches specified instance parameter list for an instance parameter that matches 
 the name of the specified instance parameter.  If a match is found, a pointer to 
 the found instance parameter is returned to the calling function; otherwise, a value of NULL
 is returned if no match was found.
*/
inst_parm* inst_parm_find( char* name, inst_parm* iparm ) { PROFILE(INST_PARM_FIND);

  assert( name != NULL );

  while( (iparm != NULL) && ((iparm->sig == NULL) || (iparm->sig->name == NULL) || (strcmp( iparm->sig->name, name ) != 0)) ) {
    iparm = iparm->next;
  }

  return( iparm );
 
}

/*!
 \param name       Name of parameter
 \param inst_name  Name of instance containing this parameter name
 \param msb        Static expression containing the MSB of this instance parameter
 \param lsb        Static expression containing the LSB of this instance parameter
 \param is_signed  Specifies if this instance parameter should be treated as signed or unsigned
 \param value      Vector value of specified instance parameter.
 \param mparm      Pointer to module instance that this instance parameter is derived from.
 \param inst       Pointer to current functional unit instance.

 \return Returns pointer to newly created instance parameter.

 Creates a new instance parameter with the specified information and adds 
 it to the instance parameter list.
*/
inst_parm* inst_parm_add( char* name, char* inst_name, static_expr* msb, static_expr* lsb, bool is_signed,
                          vector* value, mod_parm* mparm, funit_inst* inst ) { PROFILE(INST_PARM_ADD);

  inst_parm* iparm;           /* Temporary pointer to instance parameter */
  int        sig_width;       /* Width of this parameter signal */
  int        sig_lsb;         /* LSB of this parameter signal */
  int        sig_be;          /* Big endianness of this parameter signal */
  int        left_val  = 31;  /* Value of left (msb) static expression */
  int        right_val = 0;   /* Value of right (lsb) static expression */
  exp_link*  expl;            /* Pointer to current expression link */
  
  assert( value != NULL );
  assert( ((msb == NULL) && (lsb == NULL)) || ((msb != NULL) && (lsb != NULL)) );

  /* Create new signal/expression binding */
  iparm = (inst_parm*)malloc_safe( sizeof( inst_parm ) );

  if( inst_name != NULL ) {
    iparm->inst_name = strdup_safe( inst_name );
  } else {
    iparm->inst_name = NULL;
  }

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

  /* Create instance parameter signal */
  iparm->sig = vsignal_create( name, SSUPPL_TYPE_PARAM, sig_width, 0, 0 );
  iparm->sig->pdim_num   = 1;
  iparm->sig->dim        = (dim_range*)malloc_safe( sizeof( dim_range ) * 1 );
  iparm->sig->dim[0].lsb = right_val;
  iparm->sig->dim[0].msb = left_val;
  iparm->sig->suppl.part.big_endian = sig_be;

  /* Store signed attribute for this vector */
  iparm->sig->value->suppl.part.is_signed = is_signed;
  
  /* Copy the contents of the specified vector value to the signal */
  vector_set_value( iparm->sig->value, value->value, value->suppl.part.type, value->width, 0, 0 );

  iparm->mparm = mparm;
  iparm->next  = NULL;

  /* Bind the module parameter expression list to this signal */
  if( mparm != NULL ) {
    expl = mparm->exp_head;
    while( expl != NULL ) {
      expl->exp->sig = iparm->sig;
      /* Set the expression's vector to this signal's vector if we are part of a generate expression */
      if( expl->exp->suppl.part.gen_expr == 1 ) {
        expression_set_value( expl->exp, iparm->sig, inst->funit );
      }
      exp_link_add( expl->exp, &(iparm->sig->exp_head), &(iparm->sig->exp_tail) );
      expl = expl->next;
    }
  }

  /* Now add the parameter to the current expression */
  if( inst->param_head == NULL ) {
    inst->param_head = inst->param_tail = iparm;
  } else {
    inst->param_tail->next = iparm;
    inst->param_tail       = iparm;
  }

  return( iparm );

}

/*!
 \param sig   Pointer to generate signal to copy
 \param inst  Pointer to instance to add this instance parameter to 

 Creates an instance parameter for a generate variable and adds it to the
 given instance parameter list.
*/
void inst_parm_add_genvar( vsignal* sig, funit_inst* inst ) { PROFILE(INST_PARM_ADD_GENVAR);

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

}

/*!
 \param iparm  Pointer to instance parameter to bind.

 Binds the instance parameter signal to its list of expressions.  This is called
 by funit_size_elements.
*/
void inst_parm_bind( inst_parm* iparm ) { PROFILE(INST_PARM_BIND);

  exp_link* expl;  /* Pointer to current expression link in list */

  /* Bind the module parameter expression list to this signal */
  if( iparm->mparm != NULL ) {
    expl = iparm->mparm->exp_head;
    while( expl != NULL ) {
      expl->exp->sig = iparm->sig;
      expl = expl->next;
    }
  }

}


/************************************************************************************/

/*!
 \param scope  Full hierarchical reference to specified scope to change value to.
 \param value  User-specified parameter override value.

 Scans list of all parameters to make sure that specified parameter isn't already
 being set to a new value.  If no match occurs, adds the new defparam to the
 defparam list.  This function is called for each -P option to the score command.
*/
void defparam_add( char* scope, vector* value ) { PROFILE(DEFPARAM_ADD);

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
    msb.num = 31;
    msb.exp = NULL;
    lsb.num = 0;
    lsb.exp = NULL;

    (void)inst_parm_add( scope, NULL, &msb, &lsb, FALSE, value, NULL, defparam_list );

    vector_dealloc( value );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Parameter (%s) value is assigned more than once", obf_sig( scope ) );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( EXIT_FAILURE );

  }

}

/*!
 Deallocates all memory used for storing defparam information.
*/
void defparam_dealloc() { PROFILE(DEFPARAM_DEALLOC);

  if( defparam_list != NULL ) {

    /* Deallocate the instance parameters in the defparam_list structure */
    inst_parm_dealloc( defparam_list->param_head, TRUE );

    /* Now free the defparam_list structure itself */
    free_safe( defparam_list );

  }

}

/*************************************************************************************/

/*!
 \param expr  Pointer to current expression to evaluate.
 \param inst  Pointer to current instance to search.

 \return Returns a pointer to the specified value found.

 This function is called by param_expr_eval when it encounters a parameter in its
 expression tree that needs to be resolved for its value.  If the parameter is
 found, the value of that parameter is returned.  If the parameter is not found,
 an error message is displayed to the user (the user has created a module in which
 a parameter value is used without being defined).
*/
void param_find_and_set_expr_value( expression* expr, funit_inst* inst ) { PROFILE(PARAM_FIND_AND_SET_EXPR_VALUE);

  inst_parm* icurr;  /* Pointer to current instance parameter being evaluated */
    
  if( inst != NULL ) {

    icurr = inst->param_head;
    while( (icurr != NULL) && ((icurr->mparm == NULL) || (exp_link_find( expr->id, icurr->mparm->exp_head ) == NULL)) ) {
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
        snprintf( user_msg, USER_MSG_LENGTH, "Parameter used in expression but not defined in current module, line %d", expr->line );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( EXIT_FAILURE );
      }

    } else {

      /* Set the found instance parameter value to this expression */
      expression_set_value( expr, icurr->sig, inst->funit );

      /* Cause expression/signal to point to each other */
      expr->sig = icurr->sig;
      
      exp_link_add( expr, &(icurr->sig->exp_head), &(icurr->sig->exp_tail) );

    }

  }
  
}

/*!
 \param sig    Pointer to signal to search for in instance parameter list.
 \param icurr  Pointer to head of instance parameter list to search.
 
 Sizes the specified signal according to the value of the specified
 instance parameter value.
*/
void param_set_sig_size( vsignal* sig, inst_parm* icurr ) { PROFILE(PARAM_SET_SIG_SIZE);

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

}

/*************************************************************************************/

/*!
 \param inst   Pointer to instance pointing to given functional unit
 \param funit  Pointer to functional unit to size

 Recursively iterates through all functional units of given function, sizing them as
 appropriate for the purposes of static function allocation and execution.
*/
void param_size_function( funit_inst* inst, func_unit* funit ) { PROFILE(PARAM_SIZE_FUNCTION);

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

}

/*!
 \param expr  Current expression to evaluate.
 \param inst  Pointer to current instance to evaluate for.

 Recursively evaluates the specified expression tree, calculating the value of leaf nodes
 first.  If a another parameter value is encountered, lookup the value of this parameter
 in the current instance instance parameter list.  If the instance parameter cannot be
 found, we have encountered a user error; therefore, display an error message to the
 user indicating such.
*/
void param_expr_eval( expression* expr, funit_inst* inst ) { PROFILE(PARAM_EXPR_EVAL);

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
#ifdef OBSOLETE
        if( expr->value->value != NULL ) {
          free_safe( expr->value->value );
        }
        expression_create_value( expr, expr->value->width, TRUE );
#endif
        break;
    }

    /* Perform the operation */
    expression_operate( expr, NULL, &time );

  }
  
}

/*************************************************************************************/

/*!
 \param mparm  Pointer to parameter in current module to check.
 \param inst   Pointer to current instance.

 \return Returns a pointer to the newly created instance parameter or NULL if one is not created

 Looks up in the parent instance instance parameter list for overrides.  If an
 override is found, adds the new instance parameter using the value of the
 override.  If no override is found, returns NULL and does nothing.
*/
inst_parm* param_has_override( mod_parm* mparm, funit_inst* inst ) { PROFILE(PARAM_HAS_OVERRIDE);

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
           !((icurr->mparm->suppl.part.type == PARAM_TYPE_OVERRIDE) &&
             (mparm->suppl.part.type != PARAM_TYPE_DECLARED_LOCAL) &&
             ((icurr->sig->name != NULL) ? (strcmp( icurr->sig->name, mparm->name ) == 0) :
                                           (mparm->suppl.part.order == icurr->mparm->suppl.part.order )) &&
             (strcmp( mod_inst->name, icurr->inst_name ) == 0)) ) {
      icurr = icurr->next;
    }

  }

  /* If an override has been found, use this value instead of the mparm expression value */
  if( icurr != NULL ) {

    /* Add new instance parameter to current instance */
    parm = inst_parm_add( mparm->name, NULL, mparm->msb, mparm->lsb, mparm->is_signed, icurr->sig->value, mparm, inst );

  }

  return( parm );

}

/*!
 \param mparm  Pointer to module parameter to attach new instance parameter to.
 \param inst   Pointer to current instance.

 \return Returns pointer to created instance parameter or NULL if one is not created.

 Looks up specified parameter in defparam list.  If a match is found, a new
 instance parameter is created with the value of the found defparam.  If a match
 is not found, return NULL and do nothing else.
*/
inst_parm* param_has_defparam( mod_parm* mparm, funit_inst* inst ) { PROFILE(PARAM_HAS_DEFPARAM);

  inst_parm* parm = NULL;       /* Pointer newly created instance parameter (if one is created) */
  inst_parm* icurr;             /* Pointer to current defparam */
  char       parm_scope[4096];  /* Specifes full scope to parameter to find */
  char       scope[4096];       /* Scope of this instance */

  assert( mparm != NULL );
  assert( inst != NULL );

  /* Make sure that the user specified at least one defparam */
  if( defparam_list != NULL ) {

    /* Get scope of this instance */
    scope[0] = '\0';
    instance_gen_scope( scope, inst, FALSE );

    assert( leading_hier_num > 0 );

    /* Generate full hierarchy of this parameter */
    if( strcmp( leading_hierarchies[0], "*" ) == 0 ) {
      snprintf( parm_scope, 4096, "%s.%s", scope, mparm->name );
    } else {
      snprintf( parm_scope, 4096, "%s.%s.%s", leading_hierarchies[0], scope, mparm->name );
    }

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

  return( parm );

}

/*!
 \param mparm  Pointer to parameter in current module to check.
 \param inst   Pointer to current instance.

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
void param_resolve_declared( mod_parm* mparm, funit_inst* inst ) { PROFILE(PARAM_RESOLVE_DECLARED);

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

}

/************************************************************************************/

/*!
 \param oparm  Pointer to override module parameter.
 \param inst   Pointer to instance to add new instance parameter to.

 Evaluates the current module parameter expression tree and adds a new instance
 parameter to the specified instance parameter list, preserving the order and
 type of the override parameter.
*/
void param_resolve_override( mod_parm* oparm, funit_inst* inst ) { PROFILE(PARAM_RESOLVE_OVERRIDE);

  assert( oparm != NULL );

  /* If this is a NULL parameter, don't attempt an expression evaluation */
  if( oparm->expr != NULL ) {

    /* Evaluate module override parameter */
    param_expr_eval( oparm->expr, inst );

    /* Add the new instance override parameter */
    (void)inst_parm_add( oparm->name, oparm->inst_name, oparm->msb, oparm->lsb, oparm->is_signed, oparm->expr->value, oparm, inst );

  }

}

/*!
 \param inst  Pointer to functional unit instance to resolve parameter values for

 Called after binding has occurred.  Recursively resolves all parameters for the given
 instance tree.
*/
void param_resolve( funit_inst* inst ) { PROFILE(PARAM_RESOLVE);

  mod_parm*   mparm;  /* Pointer to current module parameter in functional unit */
  funit_inst* child;  /* Pointer to child instance of this instance */

  assert( inst != NULL );

  /* Resolve this instance */
  mparm = inst->funit->param_head;
  while( mparm != NULL ) {
    if( (mparm->suppl.part.type == PARAM_TYPE_DECLARED) ||
        (mparm->suppl.part.type == PARAM_TYPE_DECLARED_LOCAL) ) {
      param_resolve_declared( mparm, inst );
    } else {
      param_resolve_override( mparm, inst );
    }
    mparm = mparm->next;
  }

  /* Resolve all child instances */
  child = inst->child_head;
  while( child != NULL ) {
    param_resolve( child );
    child = child->next;
  }

}

/*!
 \param iparm       Pointer to instance parameter to output to file.
 \param file        Pointer to file handle to write parameter contents to.
 \param parse_mode  Specifies if we are writing after just parsing the design

 Prints contents of specified instance parameter to the specified output stream.
 Parameters get output in the same format as signals (they type specified for parameters
 is DB_TYPE_SIGNAL).  A leading # sign is attached to the parameter name to indicate
 that the current signal is a parameter and not a signal, and should therefore not
 be scored as a signal.
*/
void param_db_write( inst_parm* iparm, FILE* file, bool parse_mode ) { PROFILE(PARAM_DB_WRITE);

  /*
   If the parameter does not have a name, it will not be used in expressions;
   therefore, there is no reason to output this parameter to the CDD file.
  */
  if( iparm->sig->name != NULL ) {

    vsignal_db_write( iparm->sig, file );

  }

}

/**********************************************************************************/

/*!
 \param parm  Pointer to module parameter to remove
 \param recursive  If TRUE, removes entire module parameter list; otherwise, just remove me.

 Deallocates allocated memory from heap for the specified module parameter.  If
 the value of recursive is set to TRUE, perform this deallocation for the entire
 list of module parameters.
*/
void mod_parm_dealloc( mod_parm* parm, bool recursive ) { PROFILE(MOD_PARM_DEALLOC);

  if( parm != NULL ) {

    /* If the user wants to deallocate the entire module parameter list, do so now */
    if( recursive ) {
      mod_parm_dealloc( parm->next, recursive );
    }

    /* Deallocate MSB and LSB static expressions */
    static_expr_dealloc( parm->msb, FALSE );
    static_expr_dealloc( parm->lsb, FALSE );

    /* Remove the attached expression tree */
    if( parm->suppl.part.owns_expr == 1 ) {
      expression_dealloc( parm->expr, FALSE );
    }

    /* Remove the expression list that this parameter is used in */
    exp_link_delete_list( parm->exp_head, FALSE );

    /* Remove the parameter name */
    free_safe( parm->name );

    /* Remove instance name, if specified */
    free_safe( parm->inst_name );

    /* Remove the parameter itself */
    free_safe( parm );

  }

}

/*!
 \param iparm      Pointer to instance parameter to remove
 \param recursive  If TRUE, removes entire instance parameter list; otherwise, just remove me.

 Deallocates allocated memory from heap for the specified instance parameter.  If
 the value of recursive is set to TRUE, perform this deallocation for the entire
 list of instance parameters.
*/
void inst_parm_dealloc( inst_parm* iparm, bool recursive ) { PROFILE(INST_PARM_DEALLOC);

  if( iparm != NULL ) {

    /* If the user wants to deallocate the entire module parameter list, do so now */
    if( recursive ) {
      inst_parm_dealloc( iparm->next, recursive );
    }

    /* Deallocate parameter signal */
    vsignal_dealloc( iparm->sig );

    /* Deallocate instance name, if specified */
    if( iparm->inst_name != NULL ) {
      free_safe( iparm->inst_name );
    }
    
    /* Deallocate parameter itself */
    free_safe( iparm );

  }

}


/*
 $Log$
 Revision 1.96  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.95  2007/12/19 14:37:29  phase1geo
 More compiler fixes (still more to go).  Checkpointing.

 Revision 1.94  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.93  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.92  2007/09/04 22:50:50  phase1geo
 Fixed static_afunc1 issues.  Reran regressions and updated necessary files.
 Also working on debugging one remaining issue with mem1.v (not solved yet).

 Revision 1.91  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.90  2007/07/30 20:36:14  phase1geo
 Fixing rest of issues pertaining to new implementation of function calls.
 Full regression passes (with the exception of afunc1 which we do not expect
 to pass with these changes as of yet).

 Revision 1.89  2007/07/26 17:05:15  phase1geo
 Fixing problem with static functions (vector data associated with expressions
 were not being allocated).  Regressions have been run.  Only two failures
 in total still to be fixed.

 Revision 1.88  2007/07/26 05:03:42  phase1geo
 Starting to work on fix for static function support.  Fixing issue if
 func_call is called with NULL thr parameter (to avoid segmentation fault).
 IV regression fully passes.

 Revision 1.87  2007/04/18 22:35:02  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.86  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.85  2006/12/12 06:20:23  phase1geo
 More updates to support re-entrant tasks/functions.  Still working through
 compiler errors.  Checkpointing.

 Revision 1.84.2.1  2007/04/17 16:31:53  phase1geo
 Fixing bug 1698806 by rebinding a parameter signal to its list of expressions
 prior to writing the initial CDD file (elaboration phase).  Added param16
 diagnostic to regression suite to verify the fix.  Full regressions pass.

 Revision 1.84  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.83  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.82  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.81  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.80  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.79  2006/09/22 04:23:04  phase1geo
 More fixes to support new signal range structure.  Still don't have full
 regressions passing at the moment.

 Revision 1.78  2006/09/21 22:44:20  phase1geo
 More updates to regressions for latest changes to support memories/multi-dimensional
 arrays.  We still have a handful of VCS diagnostics that are failing.  Checkpointing
 for now.

 Revision 1.77  2006/09/21 04:20:59  phase1geo
 Fixing endianness diagnostics.  Still getting memory error with some diagnostics
 in regressions (ovl1 is one of them).  Updated regression.

 Revision 1.76  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.75  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.74  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.73  2006/09/04 05:28:18  phase1geo
 Fixing bug 1546059 last remaining issue.  Updated user documentation.

 Revision 1.72  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.71  2006/07/31 16:26:53  phase1geo
 Tweaking the is_static_only function to consider expressions using generate
 variables to be static.  Updating regression for changes.  Full regression
 now passes.

 Revision 1.70  2006/07/27 02:04:30  phase1geo
 Fixing problem with parameter usage in a generate block for signal sizing.

 Revision 1.69  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.68  2006/07/22 03:57:07  phase1geo
 Adding support for parameters within generate blocks.  Adding more diagnostics
 to verify statement support and parameter usage (signal sizing).

 Revision 1.67  2006/07/20 20:11:09  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.66  2006/07/10 03:05:04  phase1geo
 Contains bug fixes for memory leaks and segmentation faults.  Also contains
 some starting code to support generate blocks.  There is absolutely no
 functionality here, however.

 Revision 1.65  2006/07/08 02:06:54  phase1geo
 Updating build scripts for next development release and fixing a bug in
 the score command that caused segfaults for signals that used the same
 parameters in their range declaration.  Added diagnostic to regression
 to test this fix.  Full regression passes.

 Revision 1.64  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.63  2006/05/25 12:11:01  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.62  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.61  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.60.4.2.4.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.60.4.2  2006/04/21 04:42:02  phase1geo
 Adding endian2 and endian3 diagnostics to regression suite to verify other
 endianness related code.  Made small fix to parameter CDD output function
 to include the supplemental field output.  Full regression passes.

 Revision 1.60.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.60  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.59  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.58  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.57  2006/02/13 15:43:01  phase1geo
 Adding support for NULL expressions in parameter override expression lists.  In VCS,
 this simply skips overriding the Nth parameter -- Covered does the same.  Full
 regression now passes.

 Revision 1.56  2006/02/02 22:37:41  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.55  2006/02/01 19:58:28  phase1geo
 More updates to allow parsing of various parameter formats.  At this point
 I believe full parameter support is functional.  Regression has been updated
 which now completely passes.  A few new diagnostics have been added to the
 testsuite to verify additional functionality that is supported.

 Revision 1.54  2006/02/01 15:13:11  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.53  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.52  2006/01/25 04:32:47  phase1geo
 Fixing bug with latest checkins.  Full regression is now passing for IV simulated
 diagnostics.

 Revision 1.51  2006/01/24 23:24:38  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.50  2006/01/23 22:55:10  phase1geo
 Updates to fix constant function support.  There is some issues to resolve
 here but full regression is passing with the exception of the newly added
 static_func1.1 diagnostic.  Fixed problem where expand and multi-bit expressions
 were getting coverage numbers calculated for them before they were simulated.

 Revision 1.49  2006/01/20 22:50:50  phase1geo
 Code cleanup.

 Revision 1.48  2006/01/20 22:44:51  phase1geo
 Moving parameter resolution to post-bind stage to allow static functions to
 be considered.  Regression passes without static function testing.  Static
 function support still has some work to go.  Checkpointing.

 Revision 1.47  2006/01/20 19:27:14  phase1geo
 Fixing compile warning.

 Revision 1.46  2006/01/20 19:15:23  phase1geo
 Fixed bug to properly handle the scoping of parameters when parameters are created/used
 in non-module functional units.  Added param10*.v diagnostics to regression suite to
 verify the behavior is correct now.

 Revision 1.45  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.44  2006/01/16 17:27:41  phase1geo
 Fixing binding issues when designs have modules/tasks/functions that are either used
 more than once in a design or have the same name.  Full regression now passes.

 Revision 1.43  2006/01/12 22:53:01  phase1geo
 Adding support for localparam construct.  Added tests to regression suite to
 verify correct functionality.  Full regression passes.

 Revision 1.42  2006/01/12 22:14:45  phase1geo
 Completed code for handling parameter value pass by name Verilog-2001 syntax.
 Added diagnostics to regression suite and updated regression files for this
 change.  Full regression now passes.

 Revision 1.41  2005/12/21 23:16:53  phase1geo
 More memory leak fixes.

 Revision 1.40  2005/12/21 22:30:54  phase1geo
 More updates to memory leak fix list.  We are getting close!  Added some helper
 scripts/rules to more easily debug valgrind memory leak errors.  Also added suppression
 file for valgrind for a memory leak problem that exists in lex-generated code.

 Revision 1.39  2005/12/17 05:47:36  phase1geo
 More memory fault fixes.  Regression runs cleanly and we have verified
 no memory faults up to define3.v.  Still have a ways to go.

 Revision 1.38  2005/12/15 17:24:46  phase1geo
 More fixes for memory fault clean-up.  At this point all of the always
 diagnostics have been run and come up clean with valgrind.  Full regression
 passes.

 Revision 1.37  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.36  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.35  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.34  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.33  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.32  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.31  2003/11/29 06:55:48  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.30  2003/10/17 21:55:25  phase1geo
 Fixing parameter db_write function to output signal in new format.

 Revision 1.29  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.28  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.27  2003/02/07 02:28:23  phase1geo
 Fixing bug with statement removal.  Expressions were being deallocated but not properly
 removed from module parameter expression lists and module expression lists.  Regression
 now passes again.

 Revision 1.26  2003/01/14 05:52:17  phase1geo
 Fixing bug related to copying instance trees in modules that were previously
 parsed.  Added diagnostic param7.v to testsuite and regression.  Full
 regression passes.

 Revision 1.25  2003/01/04 03:56:28  phase1geo
 Fixing bug with parameterized modules.  Updated regression suite for changes.

 Revision 1.24  2002/12/13 16:49:48  phase1geo
 Fixing infinite loop bug with statement set_stop function.  Removing
 hierarchical references from scoring (same problem as defparam statement).
 Fixing problem with checked in version of param.c and fixing error output
 in bind() function to be more meaningful to user.

 Revision 1.23  2002/12/02 21:46:53  phase1geo
 Fixing bug in parameter file that handles parameters used in instances which are
 instantiated multiple times in the design.

 Revision 1.22  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.21  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.20  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.19  2002/10/12 06:51:34  phase1geo
 Updating development documentation to match all changes within source.
 Adding new development pages created by Doxygen for the new source
 files.

 Revision 1.18  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.17  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.16  2002/10/01 13:21:25  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.15  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.14  2002/09/27 01:19:38  phase1geo
 Fixed problems with parameter overriding from command-line.  This now works
 and param1.2.v has been added to test this functionality.  Totally reworked
 regression running to allow each diagnostic to specify unique command-line
 arguments to Covered.  Full regression passes.

 Revision 1.13  2002/09/26 13:43:45  phase1geo
 Making code adjustments to correctly support parameter overriding.  Added
 parameter tests to verify supported functionality.  Full regression passes.

 Revision 1.12  2002/09/26 04:17:11  phase1geo
 Adding support for expressions in parameter definitions.  param1.1.v added to
 test simple functionality of this and it passes regression.

 Revision 1.11  2002/09/25 05:36:08  phase1geo
 Initial version of parameter support is now in place.  Parameters work on a
 basic level.  param1.v tests this basic functionality and param1.cdd contains
 the correct CDD output from handling parameters in this file.  Yeah!

 Revision 1.10  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.9  2002/09/23 01:37:45  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.8  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.7  2002/09/21 04:11:32  phase1geo
 Completed phase 1 for adding in parameter support.  Main code is written
 that will create an instance parameter from a given module parameter in
 its entirety.  The next step will be to complete the module parameter
 creation code all the way to the parser.  Regression still passes and
 everything compiles at this point.

 Revision 1.6  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.5  2002/09/12 05:16:25  phase1geo
 Updating all CDD files in regression suite due to change in vector handling.
 Modified vectors to assign a default value of 0xaa to unassigned registers
 to eliminate bugs where values never assigned and VCD file doesn't contain
 information for these.  Added initial working version of depth feature in
 report generation.  Updates to man page and parameter documentation.

 Revision 1.4  2002/09/06 03:05:28  phase1geo
 Some ideas about handling parameters have been added to these files.  Added
 "Special Thanks" section in User's Guide for acknowledgements to people
 helping in project.

 Revision 1.3  2002/08/27 11:53:16  phase1geo
 Adding more code for parameter support.  Moving parameters from being a
 part of modules to being a part of instances and calling the expression
 operation function in the parameter add functions.

 Revision 1.2  2002/08/26 12:57:04  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.1  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.
*/

