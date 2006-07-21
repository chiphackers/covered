/*!
 \file     gen_item.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/10/2006
*/

#include <string.h>
#include <stdio.h>

#include "defines.h"
#include "gen_item.h"
#include "expr.h"
#include "util.h"
#include "vsignal.h"
#include "instance.h"
#include "statement.h"


extern funit_inst* instance_root;
extern char        user_msg[USER_MSG_LENGTH];
extern bool        debug_mode;


/*!
 \param gi  Pointer to generate item to display.

 Displays the contents of the specified generate item to standard output (used for debugging purposes).
*/
void gen_item_display( gen_item* gi ) {

  switch( gi->suppl.type ) {
    case GI_TYPE_EXPR :
      printf( "  EXPR\n" );
      expression_display( gi->elem.expr );
      break;
    case GI_TYPE_SIG :
      printf( "  SIG\n" );
      vsignal_display( gi->elem.sig );
      break;
    case GI_TYPE_STMT :
      printf( "  STMT, id: %d\n", gi->elem.stmt->exp->id );
      break;
    case GI_TYPE_INST :
      printf( "  INST, name: %s\n", gi->elem.inst->name );
      break;
    case GI_TYPE_TFN :
      printf( "  TFN, name: %s, type: %d\n", gi->elem.inst->name, gi->elem.inst->funit->type );
      break;
    case GI_TYPE_END :
      printf( "  END, upscope: %s\n", gi->elem.inst->name );
      break;
    default :
      break;
  }

}

/*!
 \param gi1  Pointer to first generate item to compare
 \param gi2  Pointer to second generate item to compare

 \return Returns TRUE if the two specified generate items are equivalent; otherwise,
         returns FALSE.
*/
bool gen_item_compare( gen_item* gi1, gen_item* gi2 ) {

  bool retval = FALSE;  /* Return value for this function */

  if( (gi1 != NULL) && (gi2 != NULL) && (gi1->suppl.type == gi2->suppl.type) ) {

    switch( gi1->suppl.type ) {
      case GI_TYPE_EXPR :  retval = (gi1->elem.expr->id == gi2->elem.expr->id) ? TRUE : FALSE;  break;
      case GI_TYPE_SIG  :  retval = scope_compare( gi1->elem.sig->name, gi2->elem.sig->name );  break;
      case GI_TYPE_STMT :  retval = (gi1->elem.stmt->exp->id == gi2->elem.stmt->exp->id) ? TRUE : FALSE;  break;
      case GI_TYPE_INST :
      case GI_TYPE_TFN  :  retval = (strcmp( gi1->elem.inst->name, gi2->elem.inst->name ) == 0) ? TRUE : FALSE;  break;
      case GI_TYPE_END  :  break;
      default           :  break;
    }

  }

  return( retval );

}

/*!
 \param root  Pointer to root of generate item block to search in
 \param gi    Pointer to generate item to search for

 \return Returns a pointer to the generate item that matches the given generate item
         in the given generate item block.  Returns NULL if it was not able to find
         a matching item.

 Recursively traverses the specified generate item block searching for a generate item
 that matches the specified generate item.
*/
gen_item* gen_item_find( gen_item* root, gen_item* gi ) {

  gen_item* found = NULL;  /* Return value for this function */

  assert( gi != NULL );

  if( root != NULL ) {

    if( gen_item_compare( root, gi ) ) {

      found = root;

    } else {

      /* If both true and false paths lead to same item, just traverse the true path */
      if( root->next_true == root->next_false ) {

        if( root->suppl.stop_true == 0 ) {
          found = gen_item_find( root->next_true, gi );
        }

      /* Otherwise, traverse both true and false paths */
      } else if( (root->suppl.stop_true == 0) && ((found = gen_item_find( root->next_true, gi )) == NULL) ) {

        if( root->suppl.stop_false == 0 ) {
          found = gen_item_find( root->next_false, gi );
        }

      }

    }

  }

  return( found );

}

/*!
 \param expr  Pointer to root expression to create (this is an expression from a FOR, IF or CASE statement)

 \return Returns a pointer to created generate item.

 Creates a new generate item for an expression.
*/
gen_item* gen_item_create_expr( expression* expr ) {

  gen_item* gi;

  /* Create the generate item for an expression */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.expr     = expr;
  gi->suppl.type    = GI_TYPE_EXPR;
  gi->suppl.conn_id = 0;
  gi->genvar        = NULL;
  gi->next_true     = NULL;
  gi->next_false    = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_expr, id: %d, op: %s, line: %d, %p",
              expr->id, expression_string_op( expr->op ), expr->line, gi );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param expr  Pointer to signal to create a generate item for (wire/reg instantions)

 \return Returns a pointer to created generate item.

 Creates a new generate item for a signal.
*/
gen_item* gen_item_create_sig( vsignal* sig ) {

  gen_item* gi;

  /* Create the generate item for a signal */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.sig      = sig;
  gi->suppl.type    = GI_TYPE_SIG;
  gi->suppl.conn_id = 0;
  gi->genvar        = NULL;
  gi->next_true     = NULL;
  gi->next_false    = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_sig, name: %s, %p", sig->name, gi );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param stmt  Pointer to statement to create a generate item for (assign, always, initial blocks)

 \return Returns a pointer to create generate item.

 Create a new generate item for a statement.
*/
gen_item* gen_item_create_stmt( statement* stmt ) {

  gen_item* gi;

  /* Create the generate item for a statement */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.stmt     = stmt;
  gi->suppl.type    = GI_TYPE_STMT;
  gi->suppl.conn_id = 0;
  gi->genvar        = NULL;
  gi->next_true     = NULL;
  gi->next_false    = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_stmt, id: %d, line: %d, %p", stmt->exp->id, stmt->exp->line, gi );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param stmt  Pointer to instance to create a generate item for (instantiations)

 \return Returns a pointer to create generate item.

 Create a new generate item for an instance.
*/
gen_item* gen_item_create_inst( funit_inst* inst ) {

  gen_item* gi;

  /* Create the generate item for an instance */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.inst     = inst;
  gi->suppl.type    = GI_TYPE_INST;
  gi->suppl.conn_id = 0;
  gi->genvar        = NULL;
  gi->next_true     = NULL;
  gi->next_false    = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_inst, name: %s, %p", inst->name, gi );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param inst  Pointer to namespace to create a generate item for (named blocks, functions or tasks)

 \return Returns a pointer to create generate item.

 Create a new generate item for a namespace.
*/
gen_item* gen_item_create_tfn( funit_inst* inst ) {

  gen_item* gi;

  /* Create the generate item for a namespace */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.inst     = inst;
  gi->suppl.type    = GI_TYPE_TFN;
  gi->suppl.conn_id = 0;
  gi->genvar        = NULL;
  gi->next_true     = NULL;
  gi->next_false    = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_tfn, name: %s, %p", inst->name, gi );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param inst  Pointer to namespace to create a generate item for (upscope)

 \return Returns a pointer to create generate item.

 Create a new generate item for the end of a scope.
*/
gen_item* gen_item_create_end( funit_inst* inst ) {

  gen_item* gi;

  /* Create the generate item for a namespace */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ), __FILE__, __LINE__ );
  gi->elem.inst     = inst;
  gi->suppl.type    = GI_TYPE_END;
  gi->suppl.conn_id = 0;
  gi->genvar        = NULL;
  gi->next_true     = NULL;
  gi->next_false    = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_end, name: %s, %p", inst->name, gi );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param gi    Pointer to current generate item to test and output
 \param type  Specifies the type of the generate item to output
 \param file  Output file to display generate item to

 Checks the given generate item against the supplied type.  If they match,
 outputs the given generate item to the specified output file.  If they do
 not match, nothing is done.
*/
void gen_item_db_write( gen_item* gi, control type, FILE* ofile ) {

  /* If the types match, output based on type */
  if( gi->suppl.type == type ) {

    switch( type ) {
      case GI_TYPE_SIG :
        vsignal_db_write( gi->elem.sig, ofile );
        break;
      case GI_TYPE_STMT :
        statement_db_write( gi->elem.stmt, ofile, TRUE );
        break;
      default :  /* Should never be called */
        assert( (type == GI_TYPE_SIG) || (type == GI_TYPE_STMT) );
        break;
    }

  }

}

/*!
 \param gi1      Pointer to generate item block to connect to gi2
 \param gi2      Pointer to generate item to connect to gi1
 \param conn_id  Connection ID

 \return Returns TRUE if the connection was successful; otherwise, returns FALSE.
*/
bool gen_item_connect( gen_item* gi1, gen_item* gi2, int conn_id ) {

  bool retval;  /* Return value for this function */

  /* Set the connection ID */
  gi1->suppl.conn_id = conn_id;

  /* If both paths go to the same destination, only parse one path */
  if( gi1->next_true == gi1->next_false ) {

    /* If the TRUE path is NULL, connect it to the new statement */
    if( gi1->next_true == NULL ) {
      gi1->next_true  = gi2;
      gi1->next_false = gi2;
      if( gi1->next_true->suppl.conn_id == conn_id ) {
        gi1->suppl.stop_true  = 1;
        gi1->suppl.stop_false = 1;
      }
      retval = TRUE;
    } else if( gi1->next_true->suppl.conn_id == conn_id ) {
      gi1->suppl.stop_true  = 1;
      gi1->suppl.stop_false = 1;

    /* If the TRUE path leads to a loop/merge, stop traversing */
    } else if( gi1->next_true != gi2 ) {
      retval |= gen_item_connect( gi1->next_true, gi2, conn_id );
    }

  } else {

    /* Traverse FALSE path */
    if( gi1->next_false == NULL ) {
      gi1->next_false = gi2;
      if( gi1->next_false->suppl.conn_id == conn_id ) {
        gi1->suppl.stop_false = 1;
      } else {
        gi1->next_false->suppl.conn_id = conn_id;
      }
      retval = TRUE;
    } else if( gi1->next_false->suppl.conn_id == conn_id ) {
      gi1->suppl.stop_false = 1;
    } else if( gi1->next_false != gi2 ) {
      retval |= gen_item_connect( gi1->next_false, gi2, conn_id );
    }

    /* Traverse TRUE path */
    if( gi1->next_true == NULL ) {
      gi1->next_true = gi2;
      if( gi1->next_true->suppl.conn_id == conn_id ) {
        gi1->suppl.stop_true = 1;
      }
      retval = TRUE;
    } else if( gi1->next_true->suppl.conn_id == conn_id ) {
      gi1->suppl.stop_true = 1;
    } else if( (gi1->next_true != gi2) && ((gi1->suppl.type != GI_TYPE_TFN) || (gi1->genvar == NULL)) ) {
      retval |= gen_item_connect( gi1->next_true, gi2, conn_id );
    }

  }

  return( retval );

}

/*!
 \param gi    Pointer to current generate item to resolve
 \param inst  Pointer to instance to store results to
*/
void gen_item_resolve( gen_item* gi, funit_inst* inst ) {

  funit_inst* child;  /* Pointer to child instance of this instance to resolve */

  if( gi != NULL ) {

#ifdef DEBUG_MODE 
    if( debug_mode ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Resolving generate item, type: %d for inst: %s", gi->suppl.type, inst->name );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    switch( gi->suppl.type ) {
  
      case GI_TYPE_EXPR :
        /* Recursively resize the expression tree if we have not already done this */
        if( gi->elem.expr->exec_num == 0 ) {
          expression_resize( gi->elem.expr, TRUE );
        }
        expression_operate_recursively( gi->elem.expr, FALSE );
        if( ESUPPL_IS_TRUE( gi->elem.expr->suppl ) ) {
          gen_item_resolve( gi->next_true, inst );
        } else {
          gen_item_resolve( gi->next_false, inst );
        }
        break;

      case GI_TYPE_SIG :
        gitem_link_add( gen_item_create_sig( gi->elem.sig ), &(inst->gitem_head), &(inst->gitem_tail) );
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_STMT :
        gitem_link_add( gen_item_create_stmt( gi->elem.stmt ), &(inst->gitem_head), &(inst->gitem_tail) );
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_INST :
        instance_parse_add( &instance_root, inst->funit, gi->elem.inst->funit, gi->elem.inst->name, gi->elem.inst->range, FALSE );
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_TFN :
        if( gi->genvar != NULL ) {
          char inst_name[4096];
          snprintf( inst_name, 4096, "%s[%d]", gi->elem.inst->name, vector_to_int( gi->genvar->value ) );
          instance_parse_add( &instance_root, inst->funit, gi->elem.inst->funit, inst_name, NULL, FALSE );
          snprintf( inst_name, 4096, "%s.%s[%d]", inst->name, gi->elem.inst->name, vector_to_int( gi->genvar->value ) );
          child = instance_find_scope( inst, inst_name );
        }
        gen_item_resolve( gi->next_true, child );
        gen_item_resolve( gi->next_false, inst );
        break;

      case GI_TYPE_END :
        gen_item_resolve( gi->next_true, gi->elem.inst );
        break;

      default :
        assert( (gi->suppl.type == GI_TYPE_EXPR) || (gi->suppl.type == GI_TYPE_SIG) ||
                (gi->suppl.type == GI_TYPE_STMT) || (gi->suppl.type == GI_TYPE_INST) ||
                (gi->suppl.type == GI_TYPE_TFN)  || (gi->suppl.type == GI_TYPE_END) );
        break;

    }

  }

}

/*!
 \param root  Pointer to current instance in instance tree to resolve for

 Recursively resolves all generate items in the design.  This is called at a specific point
 in the binding process.
*/
void generate_resolve( funit_inst* root ) {

  gitem_link* curr_gi;     /* Pointer to current gitem_link element to resolve for */
  funit_inst* curr_child;  /* Pointer to current child to resolve for */

  if( root != NULL ) {

    /* Resolve ourself */
    curr_gi = root->funit->gitem_head;
    while( curr_gi != NULL ) {
      gen_item_resolve( curr_gi->gi, root );
      curr_gi = curr_gi->next;
    }

    /* Resolve children */
    curr_child = root->child_head;
    while( curr_child != NULL ) {
      generate_resolve( curr_child );
      curr_child = curr_child->next;
    } 

  }

}

/*!
 \param gi       Pointer to gen_item structure to deallocate
 \param rm_elem  If set to TRUE, removes the associated element

 Recursively deallocates the gen_item structure tree.
*/
void gen_item_dealloc( gen_item* gi, bool rm_elem ) {

  if( gi != NULL ) {

    /* Deallocate children first */
    if( gi->next_true == gi->next_false ) {
      if( gi->suppl.stop_true == 0 ) {
        gen_item_dealloc( gi->next_true, rm_elem );
      }
    } else {
      if( gi->suppl.stop_false == 0 ) {
        gen_item_dealloc( gi->next_false, rm_elem );
      }
      if( gi->suppl.stop_true == 0 ) {
        gen_item_dealloc( gi->next_true, rm_elem );
      }
    }

    /* If we need to remove the current element, do so now */
    if( rm_elem ) {
      switch( gi->suppl.type ) {
        case GI_TYPE_EXPR :  expression_dealloc( gi->elem.expr, FALSE );    break;
        case GI_TYPE_SIG  :  vsignal_dealloc( gi->elem.sig );               break;
        case GI_TYPE_STMT :  statement_dealloc_recursive( gi->elem.stmt );  break;
        case GI_TYPE_INST :
        case GI_TYPE_TFN  :  instance_dealloc( gi->elem.inst, NULL );       break;
        default           :  break;
      }
    }

    /* Now deallocate ourselves */
    free_safe( gi );

  }

} 


/*
 $Log$
 Revision 1.8  2006/07/21 15:52:41  phase1geo
 Checking in an initial working version of the generate structure.  Diagnostic
 generate1 passes.  Still a lot of work to go before we fully support generate
 statements, but this marks a working version to enhance on.  Full regression
 passes as well.

 Revision 1.7  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.6  2006/07/20 20:11:09  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.5  2006/07/20 04:55:18  phase1geo
 More updates to support generate blocks.  We seem to be passing the parser
 stage now.  Getting segfaults in the generate_resolve code, presently.

 Revision 1.4  2006/07/18 21:52:49  phase1geo
 More work on generate blocks.  Currently working on assembling generate item
 statements in the parser.  Still a lot of work to go here.

 Revision 1.3  2006/07/18 13:37:47  phase1geo
 Fixing compile issues.

 Revision 1.2  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.1  2006/07/10 22:37:14  phase1geo
 Missed the actual gen_item files in the last submission.

*/

