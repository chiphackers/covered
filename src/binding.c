/*!
 \file     binding.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/4/2002
*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "binding.h"
#include "signal.h"
#include "expr.h"
#include "instance.h"
#include "link.h"
#include "util.h"
#include "vector.h"


sig_exp_bind* seb_head;
sig_exp_bind* seb_tail;

extern mod_inst* instance_root;
extern mod_link* mod_head;

/*!
 \param sig_name  Signal scope to bind.
 \param exp       Expression ID to bind.
 \param mod_name  Name of module containing specified expression.

 Adds the specified signal and expression to the bindings linked list.
 This bindings list will be handled after all input Verilog has been
 parsed.
*/
void bind_add( char* sig_name, expression* exp, char* mod_name ) {
  
  sig_exp_bind* seb;   /* Temporary pointer to signal/expressing binding */
  
  /* Create new signal/expression binding */
  seb           = (sig_exp_bind *)malloc_safe( sizeof( sig_exp_bind ) );
  seb->sig_name = strdup( sig_name );
  seb->mod_name = strdup( mod_name );
  seb->exp      = exp;
  seb->next     = NULL;
  
  /* Add new signal/expression binding to linked list */
  if( seb_head == NULL ) {
    seb_head = seb_tail = seb;
  } else {
    seb_tail->next = seb;
    seb_tail       = seb;
  }
  
}

/*!
 \param id  Expression ID of binding to remove.

 Removes the binding containing the expression ID of id.  This needs to
 be called before an expression is removed.
*/
void bind_remove( int id ) {

  sig_exp_bind* curr;    /* Pointer to current sig_exp_bind link       */
  sig_exp_bind* last;    /* Pointer to last sig_exp_bind link examined */

  curr = seb_head;
  last = seb_head;

  while( curr != NULL ) {

    assert( curr->exp != NULL );

    if( curr->exp->id == id ) {
      
      /* Remove this binding element */
      if( (curr == seb_head) && (curr == seb_tail) ) {
        seb_head = seb_tail = NULL;
      } else if( curr == seb_head ) {
        seb_head = seb_head->next;
      } else if( curr == seb_tail ) {
        seb_tail       = last;
        seb_tail->next = NULL;
      } else {
        last->next = curr->next;
      }

      /* Now free the binding element memory */
      free_safe( curr->sig_name );
      free_safe( curr->mod_name );
      free_safe( curr );

      curr = NULL;
      
    } else {

      last = curr;
      curr = curr->next;

    }

  }
      
}

/*!
 \param expr  Pointer to expression to set bit widths in its expression tree.

 This function is called after all signals have been bound to their respective
 expressions.  It recursively moves up the given expression tree towards the root,
 setting the expression sizes of all expressions that do not currently have a
 vector size associated with them.
*/
void bind_set_tree( expression* expr ) {

  nibble value1;     /* Value to initialize LAST element of AEDGE operation */
  int    i;          /* Loop iterator                                       */
  static int count = 0;

/*
  if( count > 70 ) {
    assert( count == 70 );
  } else {
    count++;
  }
*/

  if( expr != NULL ) {

    /* Set children before I set myself */
    if( SUPPL_OP( expr->suppl ) != EXP_OP_AEDGE ) {
      bind_set_tree( expr->left );
    }
    bind_set_tree( expr->right );

    if( expr->value->width == 0 ) {

      /* Set my size */
      switch( SUPPL_OP( expr->suppl ) ) {

        /*
         In the case of an AEDGE expression, it needs to have the size of its LAST child expression
         to be the width of its right child.
        */
        case EXP_OP_AEDGE :
          expression_create_value( expr->left, expr->right->value->width, expr->right->value->lsb );
          value1 = 0x2;
          for( i=0; i<expr->left->value->width; i++ ) {
            vector_set_value( expr->left->value, &value1, 1, 0, i );
          }
          expression_create_value( expr, 1, 0 );
          break;

        /*
         In the case of an EXPAND, we need to set the width to be the product of the value of
         the left child and the bit-width of the right child.
        */
        case EXP_OP_EXPAND :
          expression_create_value( expr, (vector_to_int( expr->left->value ) * expr->right->value->width), 0 );
          break;

        /* 
         In the case of a MULTIPLY or LIST (for concatenation) operation, its expression width must be the sum of its
         children's width.  Remove the current vector and replace it with the appropriately
         sized vector.
        */
        case EXP_OP_MULTIPLY :
        case EXP_OP_LIST :
          expression_create_value( expr, (expr->left->value->width + expr->right->value->width), 0 );
          break;

        default :
          if( (expr->left != NULL) && (expr->left->value->width > expr->right->value->width) ) {
            expression_create_value( expr, expr->left->value->width, 0 );
          } else {
            expression_create_value( expr, expr->right->value->width, 0 );
          }
          break;

      }

      if( SUPPL_IS_ROOT( expr->suppl ) == 0 ) {
        bind_set_tree( expr->parent->expr );
      }

    } else {
  
      switch( SUPPL_OP( expr->suppl ) ) {
        case EXP_OP_SIG :
        case EXP_OP_SBIT_SEL :
        case EXP_OP_MBIT_SEL :
          bind_remove( expr->id );
          break;
        default : break;
      }

    }

  }

}

/*!
 \param mode  If set to 0, searches instance tree; otherwise, searches tree.
 
 Binding is the process of setting pointers in signals and expressions to
 point to each other.  These pointers are required for scoring purposes.
 Binding is required for two purposes:
   1.  The signal that is being bound may not have been parsed (hierarchical
       referencing allows for this).
   2.  An expression does not have a pointer to a signal but rather its vector.
 In the process of binding, we go through each element of the binding list,
 finding the signal to be bound in the specified tree, adding the expression
 to the signal's expression pointer list, and setting the expression vector pointer
 to point to the signal vector.
*/
void bind( int mode ) {
  
  mod_inst*     modi;          /* Pointer to found module instance                            */
  module*       mod;           /* Pointer to found module                                     */
  mod_link*     modl;          /* Pointer to found module link                                */
  signal        sig;           /* Temporary signal used for matching                          */
  sig_link*     sigl;          /* Found signal in module                                      */
  char          scope[4096];   /* Scope of signal's parent module                             */
  char          sig_name[256]; /* Name of signal in module                                    */
  char          msg[4096];     /* Error message to display to user                            */
  expression*   curr_parent;   /* Pointer to current parent expression                        */
  int           tmp_width;     /* Temporary storage of old vector width for multiplication    */
  int           i;             /* Loop iterator                                               */
  nibble        value1;        /* Temporary holder of vector nibble data for AEDGE expr's     */
  sig_exp_bind* curr_seb;      /* Pointer to current signal/expression binding to evaluate    */
  sig_exp_bind* last_seb;      /* Temporary holder of last binding for error control purposes */

  curr_seb = seb_head;

  while( curr_seb != NULL ) {

    assert( curr_seb->exp != NULL );

    if( mode == 0 ) {

      /* Find module where signal resides */
      scope_extract_back( curr_seb->sig_name, sig_name, scope );

      if( scope[0] == '\0' ) {

        /* No scope, signal was in same module as expression so search for module name */
        mod       = module_create();
        mod->name = strdup( curr_seb->mod_name );
        modl      = mod_link_find( mod, mod_head );
        assert( modl != NULL );
        module_dealloc( mod );
        mod       = modl->mod;

      } else {
        
        /* Scope present, search for module based on scope */
        modi = instance_find_scope( instance_root, scope );

        if( modi == NULL ) {
          /* Bad hierarchical reference */
          snprintf( msg, 4096, "Undefined hierarchical reference: %s", seb_head->sig_name );
          print_output( msg, FATAL );
          exit( 1 );
        } else {
          mod = modi->mod;
        }

      }

      signal_init( &sig, sig_name, NULL );
      sigl = sig_link_find( &sig, mod->sig_head );

      if( sigl == NULL ) {
        /* Bad hierarchical reference */
        snprintf( msg, 4096, "Hierarchical reference to undefined signal: %s", seb_head->sig_name );
        print_output( msg, FATAL );
        exit( 1 );
      }

      /* Add expression to signal expression list */
      exp_link_add( curr_seb->exp, &(sigl->sig->exp_head), &(sigl->sig->exp_tail) );

      /* Make expression vector be signal vector*/
      switch( SUPPL_OP( curr_seb->exp->suppl ) ) {
        case EXP_OP_SIG :
          vector_dealloc( curr_seb->exp->value );
          curr_seb->exp->value = sigl->sig->value;
          break;
        case EXP_OP_SBIT_SEL :
          curr_seb->exp->value->value = sigl->sig->value->value;
          curr_seb->exp->value->width = 1;
          break;
        case EXP_OP_MBIT_SEL :
          curr_seb->exp->value->value = sigl->sig->value->value;
          break;
        default :
          snprintf( msg, 4096, "Internal error:  Expression with bad operation (%d) in binding function", SUPPL_OP( seb_head->exp->suppl ) );
          print_output( msg, FATAL );
          exit( 1 );
          break;
      }

    }

    curr_seb = curr_seb->next;

  }

  /*
   Now that all of the signals have been tied to their corresponding expressions, we need to traverse
   all of the affected expression trees and update the expression bit-width sizes to match.
  */
  while( seb_head != NULL ) {
    last_seb = seb_head;
    if( SUPPL_IS_ROOT( seb_head->exp->suppl ) == 0 ) {
      bind_set_tree( seb_head->exp->parent->expr );    
    } else {
      bind_remove( seb_head->exp->id );
    }
    /* Verify that we always make forward progress on eliminating the SEB list */
    assert( last_seb != seb_head );
  }
  
}

/* $Log$
/* Revision 1.8  2002/07/14 05:10:42  phase1geo
/* Added support for signal concatenation in score and report commands.  Fixed
/* bugs in this code (and multiplication).
/* */
