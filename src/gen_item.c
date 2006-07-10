/*!
 \file     gen_item.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/10/2006
*/

#include "defines.h"
#include "gen_item.h"
#include "expr.h"
#include "util.h"


void gen_item_add_expr( expression* expr ) {

  gen_item* gi;

  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.expr  = expr;
  gi->type       = GI_TYPE_EXPR;
  gi->next_true  = NULL;
  gi->next_false = NULL;

}

void gen_item_add_signal( vsignal* sig ) {

  gen_item* gi;

  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.sig   = sig;
  gi->type       = GI_TYPE_SIG;
  gi->next_true  = NULL;
  gi->next_false = NULL;

}

void gen_item_add_stmt( statement* stmt ) {

  gen_item* gi;

  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.stmt  = stmt;
  gi->type       = GI_TYPE_STMT;
  gi->next_true  = NULL;
  gi->next_false = NULL;

}

void gen_item_add_inst( funit_inst* inst ) {

  gen_item* gi;

  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.inst  = inst;
  gi->type       = GI_TYPE_INST;
  gi->next_true  = NULL;
  gi->next_false = NULL;

}

void gen_item_resolve( gen_item* gi ) {

  if( gi != NULL ) {

    switch( gi->type ) {
  
      case GI_TYPE_EXPR :
        expression_operate_recursively( gi->elem.expr );
        if( ESUPPL_IS_TRUE( gi->elem.expr->suppl ) ) {
          gen_item_resolve( gi->next_true );
        } else {
          gen_item_resolve( gi->next_false );
        }
        break;

      case GI_TYPE_SIG :
        break;

      case GI_TYPE_STMT :
        break;

      case GI_TYPE_INST :
        break;

      default :
        assert( (gi->type == GI_TYPE_EXPR) || (gi->type == GI_TYPE_SIG) ||
                (gi->type == GI_TYPE_STMT) || (gi->type == GI_TYPE_INST) );
        break;

    }

  }

}

/*!
 \param gi  Pointer to gen_item structure to deallocate

 Recursively deallocates the gen_item structure tree.
*/
void gen_item_dealloc( gen_item* gi ) {

  if( gi != NULL ) {

    /* Deallocate children first */
    gen_item_dealloc( gi->next_true );
    gen_item_dealloc( gi->next_false );

    /* Now deallocate ourselves */
    free_safe( gi );

  }

} 


/*
 $Log$
*/

