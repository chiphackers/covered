/*!
 \file     param.c
 \author   Trevor Williams  (trevorw@charter.net)
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
#include "param.h"
#include "util.h"
#include "expr.h"
#include "vector.h"
#include "link.h"
#include "vsignal.h"
#include "func_unit.h"
#include "instance.h"


/*!
 This may seem to be odd to store the defparams in a functional unit instance; however, the inst_parm_add
 function has been modified to send in a functional unit instance so we need to pass the defparam head/tail
 in a functional unit instance instead of individual head and tail pointers.  The defparams in this functional
 unit can refer to any functional unit instance, however.
*/
funit_inst* defparam_list = NULL;

extern char user_msg[USER_MSG_LENGTH];
extern char leading_hierarchy[4096];


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
mod_parm* mod_parm_find( char* name, mod_parm* parm ) {

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
void mod_parm_find_expr_and_remove( expression* exp, mod_parm* parm ) {

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
 \param expr       Expression tree for current module parameter.
 \param type       Specifies type of module parameter (declared/override).
 \param funit      Functional unit to add this module parameter to.
 \param inst_name  Name of instance (used for parameter overridding)

 \return Returns pointer to newly created module parameter.

 Creates a new module parameter with the specified information and adds 
 it to the module parameter list.
*/
mod_parm* mod_parm_add( char* scope, static_expr* msb, static_expr* lsb, expression* expr, int type, func_unit* funit, char* inst_name ) {

  mod_parm*  parm;       /* Temporary pointer to instance parameter */
  mod_parm*  curr;       /* Pointer to current module parameter for ordering purposes */
  int        order = 0;  /* Current order of parameter */
  func_unit* mod_funit;  /* Pointer to module containing this functional unit (used for ordering purposes) */
  
  assert( expr != NULL );
  assert( (type == PARAM_TYPE_DECLARED)       || 
          (type == PARAM_TYPE_DECLARED_LOCAL) ||
          (type == PARAM_TYPE_OVERRIDE)       ||
          (type == PARAM_TYPE_SIG_LSB)        ||
          (type == PARAM_TYPE_SIG_MSB)        ||
          (type == PARAM_TYPE_EXP_LSB)        ||
          (type == PARAM_TYPE_EXP_MSB) );

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
  parm = (mod_parm*)malloc_safe( sizeof( mod_parm ), __FILE__, __LINE__ );
  if( scope != NULL ) {
    parm->name = strdup_safe( scope, __FILE__, __LINE__ );
  } else {
    parm->name = NULL;
  }
  if( inst_name != NULL ) {
    parm->inst_name = strdup_safe( inst_name, __FILE__, __LINE__ );
  } else {
    parm->inst_name = NULL;
  }
  if( msb != NULL ) {
    parm->msb      = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
    parm->msb->num = msb->num;
    parm->msb->exp = msb->exp;
    if( parm->msb->exp != NULL ) {
      parm->msb->exp->suppl.part.root = 1;
    }
  } else {
    parm->msb = NULL;
  }
  if( lsb != NULL ) {
    parm->lsb      = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
    parm->lsb->num = lsb->num;
    parm->lsb->exp = lsb->exp;
    if( parm->lsb->exp != NULL ) {
      parm->lsb->exp->suppl.part.root = 1;
    }
  } else {
    parm->lsb = NULL;
  }
  parm->expr                  = expr;
  parm->expr->suppl.part.root = 1;
  parm->suppl.all             = 0;
  parm->suppl.part.type       = type;
  parm->suppl.part.order      = order;
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
    assert( mparm->expr != NULL );
    switch( mparm->suppl.part.type ) {
      case PARAM_TYPE_DECLARED       :  strcpy( type_str, "DECLARED" );        break;
      case PARAM_TYPE_OVERRIDE       :  strcpy( type_str, "OVERRIDE" );        break;
      case PARAM_TYPE_SIG_LSB        :  strcpy( type_str, "SIG_LSB"  );        break;
      case PARAM_TYPE_SIG_MSB        :  strcpy( type_str, "SIG_MSB"  );        break;
      case PARAM_TYPE_EXP_LSB        :  strcpy( type_str, "EXP_LSB"  );        break;
      case PARAM_TYPE_EXP_MSB        :  strcpy( type_str, "EXP_MSB"  );        break;
      case PARAM_TYPE_DECLARED_LOCAL :  strcpy( type_str, "DECLARED_LOCAL" );  break;
      default                        :  strcpy( type_str, "UNKNOWN" );         break;
    }
    if( mparm->name == NULL ) {
      printf( "  mparam => type: %s, order: %d, exp_id: %d\n", type_str, mparm->suppl.part.order, mparm->expr->id );
    } else {
      printf( "  mparam =>  name: %s, type: %s, order: %d, exp_id: %d\n", mparm->name, type_str, mparm->suppl.part.order, mparm->expr->id );
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
inst_parm* inst_parm_find( char* name, inst_parm* iparm ) {

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
 \param scope      Full hierarchical name of parameter value.
 \param value      Vector value of specified instance parameter.
 \param mparm      Pointer to module instance that this instance parameter is derived from.
 \param inst       Pointer to current functional unit instance.

 \return Returns pointer to newly created instance parameter.

 Creates a new instance parameter with the specified information and adds 
 it to the instance parameter list.
*/
inst_parm* inst_parm_add( char* name, char* inst_name, static_expr* msb, static_expr* lsb, vector* value, mod_parm* mparm, funit_inst* inst ) {

  inst_parm* iparm;      /* Temporary pointer to instance parameter */
  int        sig_width;  /* Width of this parameter signal */
  int        sig_lsb;    /* LSB of this parameter signal */
  int        left_val;   /* Value of left (msb) static expression */
  int        right_val;  /* Value of right (lsb) static expression */
  
  assert( value != NULL );
  assert( ((msb == NULL) && (lsb == NULL)) || ((msb != NULL) && (lsb != NULL)) );

  /* Create new signal/expression binding */
  iparm = (inst_parm*)malloc_safe( sizeof( inst_parm ), __FILE__, __LINE__ );

  if( inst_name != NULL ) {
    iparm->inst_name = strdup_safe( inst_name, __FILE__, __LINE__ );
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
    } else {
      sig_lsb   = right_val;
      sig_width = (left_val - right_val) + 1;
    }

  } else {

    sig_lsb   = 0;
    sig_width = value->width;

  }

  /* If the parameter is sized too big, panic */
  assert( (sig_width <= MAX_BIT_WIDTH) && (sig_width >= 0) );

  /* Create instance parameter signal */
  iparm->sig = vsignal_create( name, SSUPPL_TYPE_DECLARED, sig_width, sig_lsb, 0, 0 );
  
  /* Copy the contents of the specified vector value to the signal */
  vector_set_value_only( iparm->sig->value, value->value, value->width, 0, 0 );

  iparm->mparm = mparm;
  iparm->next  = NULL;

  /* Now add the parameter to the current expression */
  if( inst->param_head == NULL ) {
    inst->param_head = inst->param_tail = iparm;
  } else {
    inst->param_tail->next = iparm;
    inst->param_tail       = iparm;
  }

  return( iparm );

}


/************************************************************************************/

/*!
 \param scope  Full hierarchical reference to specified scope to change value to.
 \param value  User-specified parameter override value.

 Scans list of all parameters to make sure that specified parameter isn't already
 being set to a new value.  If no match occurs, adds the new defparam to the
 defparam list.  This function is called for each -P option to the score command.
*/
void defparam_add( char* scope, vector* value ) {

  static_expr msb;  /* MSB of this defparam (forced to be 31) */
  static_expr lsb;  /* LSB of this defparam (forced to be 0) */

  assert( scope != NULL );

  /* If the defparam instance doesn't exist, create it now */
  if( defparam_list == NULL ) {
    defparam_list = (funit_inst*)malloc_safe( sizeof( funit_inst ), __FILE__, __LINE__ );
    defparam_list->param_head = NULL;
    defparam_list->param_tail = NULL;
  }

  if( inst_parm_find( scope, defparam_list->param_head ) == NULL ) {

    /* Generate MSB and LSB information */
    msb.num = 31;
    msb.exp = NULL;
    lsb.num = 0;
    lsb.exp = NULL;

    inst_parm_add( scope, NULL, &msb, &lsb, value, NULL, defparam_list );

    vector_dealloc( value );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Parameter (%s) value is assigned more than once", scope );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );

  }

}

/*!
 Deallocates all memory used for storing defparam information.
*/
void defparam_dealloc() {

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
void param_find_and_set_expr_value( expression* expr, funit_inst* inst ) {

  inst_parm* icurr;  /* Pointer to current instance parameter being evaluated */
    
  if( inst != NULL ) {

    icurr = inst->param_head;
    while( (icurr != NULL) && (exp_link_find( expr, icurr->mparm->exp_head ) == NULL) ) {
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
        exit( 1 );
      }

    } else {
  
      /* Set the found instance parameter value to this expression */
      expression_set_value( expr, icurr->sig->value );

      /* Cause expression/signal to point to each other */
      expr->sig = icurr->sig;
      
      exp_link_add( expr, &(icurr->sig->exp_head), &(icurr->sig->exp_tail) );

    }

  }
  
}

/*!
 \param sig    Pointer to signal to search for in instance parameter list.
 \param icurr  Pointer to head of instance parameter list to search.
 
 \return Returns TRUE if signal size is fully established; otherwise, returns FALSE.

 Sizes the specified signal according to the value of the specified
 instance parameter value.
*/
bool param_set_sig_size( vsignal* sig, inst_parm* icurr ) {

  bool    established = FALSE;  /* Specifies if current signal size is fully established */
  int     bit_sel;              /* MSB/LSB bit select value from instance parameter */
  vector* vec;                  /* Pointer to new signal vector */

  assert( sig != NULL );
  assert( sig->name != NULL );

  bit_sel = vector_to_int( icurr->sig->value );

  /* LSB gets set to first value found, we may adjust this later. */
  if( sig->lsb == -1 ) {
    
    sig->lsb = bit_sel;
    
  } else {
    
    /* LSB is known so adjust the LSB and width with this value */
    if( sig->lsb <= bit_sel ) {
      sig->value->width = (bit_sel - sig->lsb) + 1;
    } else {
      sig->value->width = (sig->lsb - bit_sel) + 1;
      sig->lsb          = bit_sel;
    }

    /* Create the vector data for this signal */
    vec = vector_create( sig->value->width, TRUE );

    /* Deallocate and reassign the new data */
    if( sig->value->value != NULL ) {
      free_safe( sig->value->value );
    }
    sig->value->value = vec->value;

    /* Deallocate the vector memory */
    free_safe( vec );
    
    established = TRUE;
    
  }
  
  return( established );

}

/*************************************************************************************/

/*!
 \param expr  Current expression to evaluate.
 \param inst  Pointer to current instance to evaluate for.

 Recursively evaluates the specified expression tree, calculating the value of leaf nodes
 first.  If a another parameter value is encountered, lookup the value of this parameter
 in the current instance instance parameter list.  If the instance parameter cannot be
 found, we have encountered a user error; therefore, display an error message to the
 user indicating such.
*/
void param_expr_eval( expression* expr, funit_inst* inst ) {

  funit_inst* funiti;      /* Pointer to static function instance */
  func_unit*  funit;       /* Pointer to constant function */
  int         ignore = 0;  /* Number of instances to ignore */

  if( expr != NULL ) {

    /* For constant functions, resolve parameters and resize the functional unit first */
    if( expr->op == EXP_OP_FUNC_CALL ) {
      funit = funit_find_by_id( expr->stmt->exp->id );
      assert( funit != NULL );
      funiti = instance_find_by_funit( inst, funit, &ignore );
      assert( funiti != NULL );
      param_resolve( funiti );
      funit_size_elements( funit, funiti );
    }

    /* Evaluate children first */
    param_expr_eval( expr->left,  inst );
    param_expr_eval( expr->right, inst );

    switch( expr->op ) {
      case EXP_OP_STATIC    :
      case EXP_OP_FUNC_CALL :
        break;
      case EXP_OP_PARAM          :
      case EXP_OP_PARAM_SBIT     :
      case EXP_OP_PARAM_MBIT     :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        param_find_and_set_expr_value( expr, inst );
        break;
      default :
        /*
         Since we are not a parameter identifier, let's allocate some data for us 
         if we don't have some already.
        */
        assert( expr->value != NULL );
        assert( (expr->op != EXP_OP_SIG)      &&
                (expr->op != EXP_OP_SBIT_SEL) &&
                (expr->op != EXP_OP_MBIT_SEL) &&
                (expr->op != EXP_OP_MBIT_POS) &&
                (expr->op != EXP_OP_MBIT_NEG) );
        expression_resize( expr, FALSE );
        if( expr->value->value != NULL ) {
          free_safe( expr->value->value );
        }
        expression_create_value( expr, expr->value->width, TRUE );
        break;
    }

    /* Perform the operation */
    expression_operate( expr, NULL );

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
inst_parm* param_has_override( mod_parm* mparm, funit_inst* inst ) {

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
    parm = inst_parm_add( mparm->name, NULL, mparm->msb, mparm->lsb, icurr->sig->value, mparm, inst );

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
inst_parm* param_has_defparam( mod_parm* mparm, funit_inst* inst ) {

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
    instance_gen_scope( scope, inst );

    /* Generate full hierarchy of this parameter */
    if( strcmp( leading_hierarchy, "*" ) == 0 ) {
      snprintf( parm_scope, 4096, "%s.%s", scope, mparm->name );
    } else {
      snprintf( parm_scope, 4096, "%s.%s.%s", leading_hierarchy, scope, mparm->name );
    }

    icurr = defparam_list->param_head;
    while( (icurr != NULL) &&
           !((strcmp( icurr->sig->name, parm_scope ) == 0) &&
             (mparm->suppl.part.type != PARAM_TYPE_DECLARED_LOCAL)) ) {
      icurr = icurr->next;
    }

    if( icurr != NULL ) {

      /* Defparam found, use its value to create new instance parameter */
      parm = inst_parm_add( mparm->name, NULL, mparm->msb, mparm->lsb, icurr->sig->value, mparm, inst );

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
void param_resolve_declared( mod_parm* mparm, funit_inst* inst ) {

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
    inst_parm_add( mparm->name, NULL, mparm->msb, mparm->lsb, mparm->expr->value, mparm, inst );

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
void param_resolve_override( mod_parm* oparm, funit_inst* inst ) {

  assert( oparm       != NULL );
  assert( oparm->expr != NULL );

  /* Evaluate module override parameter */
  param_expr_eval( oparm->expr, inst );

  /* Add the new instance override parameter */
  inst_parm_add( oparm->name, oparm->inst_name, oparm->msb, oparm->lsb, oparm->expr->value, oparm, inst );

}

/*!
 \param funit  Pointer to functional unit to resolve parameter values

 Called after binding has occurred.  Recursively resolves all parameters for the given
 instance tree.
*/
void param_resolve( funit_inst* inst ) {

  mod_parm*   mparm;  /* Pointer to current module parameter in functional unit */
  funit_inst* child;  /* Pointer to child instance of this instance */

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
void param_db_write( inst_parm* iparm, FILE* file, bool parse_mode ) {

  exp_link* curr;  /* Pointer to current expression link element */

  /*
   If the parameter does not have a name, it will not be used in expressions;
   therefore, there is no reason to output this parameter to the CDD file.
  */
  if( iparm->sig->name != NULL ) {

    /* Display identification and value information first */
    fprintf( file, "%d #%s %d 0 0 ",
      DB_TYPE_SIGNAL,
      iparm->sig->name,
      iparm->sig->lsb
    );

    vector_db_write( iparm->sig->value, file, TRUE );

    curr = iparm->mparm->exp_head;
    while( curr != NULL ) {
      if( curr->exp->line != 0 ) {
        fprintf( file, " %d", expression_get_id( curr->exp, parse_mode ) );
      }
      curr = curr->next;
    }

    fprintf( file, "\n" );

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
void mod_parm_dealloc( mod_parm* parm, bool recursive ) {

  if( parm != NULL ) {

    /* If the user wants to deallocate the entire module parameter list, do so now */
    if( recursive ) {
      mod_parm_dealloc( parm->next, recursive );
    }

    /* Deallocate MSB and LSB static expressions */
    static_expr_dealloc( parm->msb, FALSE );
    static_expr_dealloc( parm->lsb, FALSE );

    /* Remove the attached expression tree */
    expression_dealloc( parm->expr, FALSE );

    /* Remove the expression list that this parameter is used in */
    exp_link_delete_list( parm->exp_head, FALSE );

    /* Remove the parameter name */
    free_safe( parm->name );

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
void inst_parm_dealloc( inst_parm* iparm, bool recursive ) {

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

