/*!
 \file     param.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/22/2002

 The following is a list of the goals that I would ideally like for parameters:

 -#  All parameter information is encapsulated in a module structure (nothing in the
     instance structures).

 -#  Parameter support should have a minimal impact on parsing performance.

 -#  All parameter information should be handled by the time that the parsed information
     is initially output to the CDD file.

 -#  The defparam statement will be ignored for all found statements and a warning generated
     to standard error for all found defparams.

 -#  Parameter overloading will be allowed at the instance declaration for instances that
     are deemed to be parsed.

 -#  A Verilog module need only be parsed once.

 -#  No module copying is to occur.

 -#  Parameter structure is to be as condensed as possible.  A parameter structure should
     contain at least the following information:
     - Name of parameter in module OR relative hierarchical name of parameter in submodule.
     - Pointer to vector containing value of parameter or parameter override (might require
       expression tree instead of vector -- root of expression tree eventually contains
       vector value of parameter).

 -#  Any code allowed for assigning parameters is allowed (with the noted exception of the
     defparam statement).


 So how do all of these goals get met?

 -#  All parameter information can be stored in the module structure by storing parameter
     assignments and parameter overloads (for submodules) in current module.

 -#  Both signals and expressions will need to change the way that they write themselves to
     the CDD file.  If a signal's width/lsb is determined with parameters, the signal and
     associated expressions will need to have width's and/or lsb's reset to new values.
     If an expression's value is determined with parameters, the expression will need
     to have width's reset to new values.

 -#  If all Verilog information can be contained in the module structure only, it will not
     be necessary to reparse a module or to copy an existing module.

 -#  It is important to note that parameters can have chain reactions in submodule evaluations
     due to instance parameter overriding of parameters that are used to override other
     submodule instance parameters.  Therefore, all parameters will need to be evaluated in
     a top-down manner (root module will need to be evaluated first followed by its children, etc.)

 -#  All defparam values supplied by the user must be static values (no expressions allowed).

 -#  Defparam overrides should be applied before default values to eliminate unnecessary default
     parameter expression evaluation.

 -#  Expression trees need to be stored in module parameter lists for parameter expressions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "defines.h"
#include "param.h"
#include "util.h"
#include "expr.h"
#include "vector.h"
#include "link.h"


inst_parm* defparam_head = NULL;   /*!< Pointer to head of parameter list for global defparams */
inst_parm* defparam_tail = NULL;   /*!< Pointer to tail of parameter list for global defparams */


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

  while( (parm != NULL) && (strcmp( parm->name, name ) != 0) ) {
    parm = parm->next;
  }

  return( parm );
 
}

/*!
 \param scope  Full hierarchical name of parameter value.
 \param expr   Expression tree for current module parameter.
 \param type   Specifies type of module parameter (declared/override).
 \param head   Pointer to head of module parameter list to add to.
 \param tail   Pointer to tail of module parameter list to add to.

 \return Returns pointer to newly created module parameter.

 Creates a new module parameter with the specified information and adds 
 it to the module parameter list.
*/
mod_parm* mod_parm_add( char* scope, expression* expr, int type, mod_parm** head, mod_parm** tail ) {

  mod_parm* parm;    /* Temporary pointer to instance parameter                   */
  mod_parm* curr;    /* Pointer to current module parameter for ordering purposes */
  int       order;   /* Current order of parameter                                */
  
  assert( scope != NULL );
  assert( expr != NULL );
  assert( (type == PARAM_TYPE_DECLARED) || (type == PARAM_TYPE_OVERRIDE) );

  /* Determine parameter order */
  if( type == PARAM_TYPE_DECLARED ) {
    curr  = *head;
    order = 0;
    while( curr != NULL ) {
      if( PARAM_TYPE( curr ) == PARAM_TYPE_DECLARED ) {
        order++;
      }
      curr = curr->next;
    }
  } else {
    curr  = *head;
    order = 0;
    while( curr != NULL ) {
      if( (PARAM_TYPE( curr ) == PARAM_TYPE_OVERRIDE) &&          (strcmp( scope, curr->name ) == 0) ) {
        order++;
      }
      curr = curr->next;
    }
  }

  /* Create new signal/expression binding */
  parm           = (mod_parm*)malloc_safe( sizeof( mod_parm ) );
  parm->name     = strdup( scope );
  parm->expr     = expr;
  parm->suppl    = ((type & 0x1) << PARAM_LSB_TYPE) | ((order & 0xffff) << PARAM_LSB_ORDER);
  parm->exp_head = NULL;
  parm->exp_tail = NULL;
  parm->sig_head = NULL;
  parm->sig_tail = NULL;
  parm->next     = NULL;

  /* Now add the parameter to the current expression */
  if( *head == NULL ) {
    *head = *tail = parm;
  } else {
    (*tail)->next = parm;
    *tail         = parm;
  }

  return( parm );

}

/*!
 \param mparm  Pointer to module parameter list to display.

 Outputs contents of specified module parameter to standard output.
 For debugging purposes only.
*/
void mod_parm_display( mod_parm* mparm ) {

  while( mparm != NULL ) {
    assert( mparm->expr != NULL );
    printf( "  mparam =>  name: %s, suppl: %d, exp_id: %d\n", mparm->name, mparm->suppl, mparm->expr->id );
    printf( "    " );  sig_link_display( mparm->sig_head );
    printf( "    " );  exp_link_display( mparm->exp_head );
    mparm = mparm->next;
  }

}

/*******************************************************************************/

/*!
 \param name  Name of parameter value to find.
 \param parm  Pointer to head of instance parameter list to search.

 \return Returns pointer to found instance parameter or NULL if instance parameter is not
         found.

 Searches specified instance parameter list for an instance parameter that matches 
 the name of the specified instance parameter.  If a match is found, a pointer to 
 the found instance parameter is returned to the calling function; otherwise, a value of NULL
 is returned if no match was found.
*/
inst_parm* inst_parm_find( char* name, inst_parm* parm ) {

  assert( name != NULL );

  while( (parm != NULL) && (strcmp( parm->name, name ) != 0) ) {
    parm = parm->next;
  }

  return( parm );
 
}

/*!
 \param scope  Full hierarchical name of parameter value.
 \param value  Vector value of specified instance parameter.
 \param mparm  Pointer to module instance that this instance parameter is derived from.
 \param head   Pointer to head of instance parameter list to add to.
 \param tail   Pointer to tail of instance parameter list to add to.

 \return Returns pointer to newly created instance parameter.

 Creates a new instance parameter with the specified information and adds 
 it to the instance parameter list.
*/
inst_parm* inst_parm_add( char* scope, vector* value, mod_parm* mparm, inst_parm** head, inst_parm** tail ) {

  inst_parm* parm;    /* Temporary pointer to instance parameter */
  
  assert( scope != NULL );
  assert( value != NULL );

  /* Create new signal/expression binding */
  parm        = (inst_parm*)malloc_safe( sizeof( inst_parm ) );
  parm->name  = strdup( scope );
  parm->value = value;
  parm->mparm = mparm;
  parm->next  = NULL;

  /* Now add the parameter to the current expression */
  if( *head == NULL ) {
    *head = *tail = parm;
  } else {
    (*tail)->next = parm;
    *tail         = parm;
  }

  return( parm );

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

  char err_msg[4096];  /* Error message to display to user */

  assert( scope != NULL );

  if( inst_parm_find( scope, defparam_head ) == NULL ) {

    inst_parm_add( scope, value, NULL, &defparam_head, &defparam_tail );

  } else {

    snprintf( err_msg, 4096, "Parameter (%s) value is assigned more than once", scope );
    print_output( err_msg, FATAL );
    exit( 1 );

  }

}

/*************************************************************************************/

/*!
 \param expr  Pointer to current expression to evaluate.
 \param icurr  Pointer to head of instance parameter list to search.

 \return Returns a pointer to the specified value found.

 This function is called by param_expr_eval when it encounters a parameter in its
 expression tree that needs to be resolved for its value.  If the parameter is
 found, the value of that parameter is returned.  If the parameter is not found,
 an error message is displayed to the user (the user has created a module in which
 a parameter value is used without being defined).
*/
vector* param_find_value_for_expr( expression* expr, inst_parm* icurr ) {

  char err_msg[4096];  /* Error message to user */

  assert( expr != NULL );
  assert( (SUPPL_OP( expr->suppl ) == EXP_OP_PARAM)      ||
          (SUPPL_OP( expr->suppl ) == EXP_OP_PARAM_SBIT) ||
          (SUPPL_OP( expr->suppl ) == EXP_OP_PARAM_MBIT) );

  while( (icurr != NULL) && (exp_link_find( expr, icurr->mparm->exp_head ) == NULL) ) {
    icurr = icurr->next;
  }

  if( icurr == NULL ) {
    snprintf( err_msg, 4096, "Parameter used in expression but not defined in current module, line %d", expr->line );
    print_output( err_msg, FATAL );
    exit( 1 );
  }

  assert( icurr->value != NULL );

  return( icurr->value );

}

/*************************************************************************************/

/*!
 \param expr   Current expression to evaluate.
 \param ihead  Pointer to head of current instance instance parameter list.

 Recursively evaluates the specified expression tree, calculating the value of leaf nodes
 first.  If a another parameter value is encountered, lookup the value of this parameter
 in the current instance instance parameter list.  If the instance parameter cannot be
 found, we have encountered a user error; therefore, display an error message to the
 user indicating such.
*/
void param_expr_eval( expression* expr, inst_parm* ihead ) {

  bool param_op;  /* If TRUE, current expression operation is a parameter operation */

  if( expr != NULL ) {

    /* Evaluate children first */
    if( (expr->left == NULL) && (expr->right == NULL) ) {
      expression_resize( expr );
    } else {
      param_expr_eval( expr->left,  ihead );
      param_expr_eval( expr->right, ihead );
    }



    param_op = ( (SUPPL_OP( expr->suppl ) == EXP_OP_PARAM)      ||
                 (SUPPL_OP( expr->suppl ) == EXP_OP_PARAM_SBIT) ||
                 (SUPPL_OP( expr->suppl ) == EXP_OP_PARAM_MBIT) );

    if( param_op ) {

      /* Get the parameter value */
      expr->value = param_find_value_for_expr( expr, ihead );

      /* Temporarily swap the operations */
      switch( SUPPL_OP( expr->suppl ) ) {
        case EXP_OP_PARAM :  
          expr->suppl = (expr->suppl & ~(0x7f << SUPPL_LSB_OP)) | (EXP_OP_SIG << SUPPL_LSB_OP);
          break;
        case EXP_OP_PARAM_SBIT :
          expr->suppl = (expr->suppl & ~(0x7f << SUPPL_LSB_OP)) | (EXP_OP_SBIT_SEL << SUPPL_LSB_OP);
          break;
        case EXP_OP_PARAM_MBIT :
          expr->suppl = (expr->suppl & ~(0x7f << SUPPL_LSB_OP)) | (EXP_OP_MBIT_SEL << SUPPL_LSB_OP);
          break;
        default :  break;
      }

    } else {

      /*
       Since we are not a parameter identifier, let's allocate some data for us 
       if we don't have some already.
      */
      assert( expr->value != NULL );
      if( expr->value->value == NULL ) {
        expression_create_value( expr, expr->value->width, expr->value->lsb, TRUE );
      }

    }

    /* Perform the operation */
    expression_operate( expr );

    if( param_op ) {

      /* Switch the new values back to the old values */
      switch( SUPPL_OP( expr->suppl ) ) {
        case EXP_OP_SIG :
          expr->suppl = (expr->suppl & ~(0x7f << SUPPL_LSB_OP)) | (EXP_OP_PARAM << SUPPL_LSB_OP);
          break;
        case EXP_OP_SBIT_SEL :
          expr->suppl = (expr->suppl & ~(0x7f << SUPPL_LSB_OP)) | (EXP_OP_PARAM_SBIT << SUPPL_LSB_OP);
          break;
        case EXP_OP_MBIT_SEL :
          expr->suppl = (expr->suppl & ~(0x7f << SUPPL_LSB_OP)) | (EXP_OP_PARAM_MBIT << SUPPL_LSB_OP);
          break;
        default :  
          assert( (SUPPL_OP( expr->suppl ) == EXP_OP_SIG)      ||
                  (SUPPL_OP( expr->suppl ) == EXP_OP_SBIT_SEL) ||
                  (SUPPL_OP( expr->suppl ) == EXP_OP_MBIT_SEL) );
          break;
      }

    }

  }
  
}

/*************************************************************************************/

/*!
 \param mname    Instance name of current module.
 \param mparm    Pointer to parameter in current module to check.
 \param ip_head  Pointer to parent instance's instance parameter list.
 \param ihead    Pointer to current instance parameter list head to add to.
 \param itail    Pointer to current instance parameter list tail to add to.

 \return Returns a pointer to the newly created instance parameter or NULL if one is not created

 Looks up in the parent instance instance parameter list for overrides.  If an
 override is found, adds the new instance parameter using the value of the
 override.  If no override is found, returns NULL and does nothing.
*/
inst_parm* param_has_override( char* mname, mod_parm* mparm, inst_parm* ip_head, 
                               inst_parm** ihead, inst_parm** itail ) {

  inst_parm* icurr;        /* Pointer to current instance parameter                  */
  inst_parm* parm = NULL;  /* Pointer to newly created parameter (if one is created) */

  assert( mname != NULL );
  assert( mparm != NULL );

  /* First, check to see if the parent instance contains an override in its instance list. */
  icurr = ip_head;
  while( (icurr != NULL) && 
         ((PARAM_TYPE( mparm ) != PARAM_TYPE_OVERRIDE)   || 
          (PARAM_ORDER( mparm ) != PARAM_ORDER( icurr->mparm )) ||
          (strcmp( mname, icurr->name ) != 0)) ) {
    icurr = icurr->next;
  }

  /* If an override has been found, use this value instead of the mparm expression value */
  if( icurr != NULL ) {

    /* Add new instance parameter to current instance */
    parm = inst_parm_add( mparm->name, icurr->value, mparm, ihead, itail );

  }
   
  return( parm );

}

/*!
 \param scope  Full hierarchical scope of parameter to check.
 \param name   Parameter name.
 \param ihead  Pointer to head of instance parameter list to add to.
 \param itail  Pointer to tail of instance parameter list to add to.

 \return Returns pointer to created instance parameter or NULL if one is not created.

 Looks up specified parameter in defparam list.  If a match is found, a new
 instance parameter is created with the value of the found defparam.  If a match
 is not found, return NULL and do nothing else.
*/
inst_parm* param_has_defparam( char* scope, char* name, inst_parm** ihead, inst_parm** itail ) {

  inst_parm* parm = NULL; /* Pointer newly created instance parameter (if one is created) */
  inst_parm* icurr;       /* Pointer to current defparam                                  */

  assert( scope != NULL );

  icurr = defparam_head;
  while( (icurr != NULL) && (strcmp( icurr->name, scope ) != 0) ) {
    icurr = icurr->next;
  }

  if( icurr != NULL ) {

    /* Defparam found, use its value to create new instance parameter */
    parm = inst_parm_add( name, icurr->value, NULL, ihead, itail );

  }

  return( parm );

}

/*!
 \param mscope   Full hierarchical scope of current module.
 \param mparm    Pointer to parameter in current module to check.
 \param ip_head  Pointer to parent instance's instance parameter list.
 \param ihead    Pointer to current instance parameter list head to add to.
 \param itail    Pointer to current instance parameter list tail to add to.

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
void param_resolve_declared( char* mscope, mod_parm* mparm, inst_parm* ip_head, 
                             inst_parm** ihead, inst_parm** itail ) {

  char* mname;  /* String containing instance name of current module   */
  char* rest;   /* String containing scope of current module -- unused */
 
  assert( mscope != NULL );
  assert( mparm != NULL );

  /* Extract the current module parameter name from its full scope */
  mname = strdup( mscope );
  rest  = strdup( mscope );
  scope_extract_back( mscope, mname, rest );

  if( param_has_override( mname, mparm, ip_head, ihead, itail ) != NULL ) {

    /* Parameter override was found in parent module, do nothing more */

  } else if( param_has_defparam( mscope, mparm->name, ihead, itail ) != NULL ) {

    /* Parameter defparam override was found, do nothing more */

  } else {
    
    assert( mparm->expr != NULL );

    /* First evaluate the current module expression */
    param_expr_eval( mparm->expr, *ihead );

    /* Now add the new instance parameter */
    inst_parm_add( mparm->name, mparm->expr->value, mparm, ihead, itail );

  }

  /* Deallocate temporary string values */
  free_safe( mname );
  free_safe( rest  );
    
}

/************************************************************************************/

/*!
 \param oparm  Pointer to override module parameter.
 \param ihead  Pointer to head of instance parameter list to add to.
 \param itail  Pointer to tail of instance parameter list to add to.

 Evaluates the current module parameter expression tree and adds a new instance
 parameter to the specified instance parameter list, preserving the order and
 type of the override parameter.
*/
void param_resolve_override( mod_parm* oparm, inst_parm** ihead, inst_parm** itail ) {

  assert( oparm       != NULL );
  assert( oparm->expr != NULL );

  /* Evaluate module override parameter */
  param_expr_eval( oparm->expr, *ihead );

  /* Add the new instance override parameter */
  inst_parm_add( oparm->name, oparm->expr->value, oparm, ihead, itail );

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

    free_safe( parm->name );

    exp_link_delete_list( parm->exp_head, FALSE );
    sig_link_delete_list( parm->sig_head );

    free_safe( parm );

  }

}

/*!
 \param parm       Pointer to instance parameter to remove
 \param recursive  If TRUE, removes entire instance parameter list; otherwise, just remove me.

 Deallocates allocated memory from heap for the specified instance parameter.  If
 the value of recursive is set to TRUE, perform this deallocation for the entire
 list of instance parameters.
*/
void inst_parm_dealloc( inst_parm* parm, bool recursive ) {

  if( parm != NULL ) {

    /* If the user wants to deallocate the entire module parameter list, do so now */
    if( recursive ) {
      inst_parm_dealloc( parm->next, recursive );
    }

    free_safe( parm->name );
    free_safe( parm );

  }

}


/* $Log$
/* Revision 1.11  2002/09/25 05:36:08  phase1geo
/* Initial version of parameter support is now in place.  Parameters work on a
/* basic level.  param1.v tests this basic functionality and param1.cdd contains
/* the correct CDD output from handling parameters in this file.  Yeah!
/*
/* Revision 1.10  2002/09/25 02:51:44  phase1geo
/* Removing need of vector nibble array allocation and deallocation during
/* expression resizing for efficiency and bug reduction.  Other enhancements
/* for parameter support.  Parameter stuff still not quite complete.
/*
/* Revision 1.9  2002/09/23 01:37:45  phase1geo
/* Need to make some changes to the inst_parm structure and some associated
/* functionality for efficiency purposes.  This checkin contains most of the
/* changes to the parser (with the exception of signal sizing).
/*
/* Revision 1.8  2002/09/21 07:03:28  phase1geo
/* Attached all parameter functions into db.c.  Just need to finish getting
/* parser to correctly add override parameters.  Once this is complete, phase 3
/* can start and will include regenerating expressions and signals before
/* getting output to CDD file.
/*
/* Revision 1.7  2002/09/21 04:11:32  phase1geo
/* Completed phase 1 for adding in parameter support.  Main code is written
/* that will create an instance parameter from a given module parameter in
/* its entirety.  The next step will be to complete the module parameter
/* creation code all the way to the parser.  Regression still passes and
/* everything compiles at this point.
/*
/* Revision 1.6  2002/09/19 05:25:19  phase1geo
/* Fixing incorrect simulation of static values and fixing reports generated
/* from these static expressions.  Also includes some modifications for parameters
/* though these changes are not useful at this point.
/*
/* Revision 1.5  2002/09/12 05:16:25  phase1geo
/* Updating all CDD files in regression suite due to change in vector handling.
/* Modified vectors to assign a default value of 0xaa to unassigned registers
/* to eliminate bugs where values never assigned and VCD file doesn't contain
/* information for these.  Added initial working version of depth feature in
/* report generation.  Updates to man page and parameter documentation.
/*
/* Revision 1.4  2002/09/06 03:05:28  phase1geo
/* Some ideas about handling parameters have been added to these files.  Added
/* "Special Thanks" section in User's Guide for acknowledgements to people
/* helping in project.
/*
/* Revision 1.3  2002/08/27 11:53:16  phase1geo
/* Adding more code for parameter support.  Moving parameters from being a
/* part of modules to being a part of instances and calling the expression
/* operation function in the parameter add functions.
/*
/* Revision 1.2  2002/08/26 12:57:04  phase1geo
/* In the middle of adding parameter support.  Intermediate checkin but does
/* not break regressions at this point.
/*
/* Revision 1.1  2002/08/23 12:55:33  phase1geo
/* Starting to make modifications for parameter support.  Added parameter source
/* and header files, changed vector_from_string function to be more verbose
/* and updated Makefiles for new param.h/.c files.
/* */

