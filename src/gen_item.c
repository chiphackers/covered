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
 \file     gen_item.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     7/10/2006
*/

#include <string.h>
#include <stdio.h>

#include "binding.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "instance.h"
#include "link.h"
#include "obfuscate.h"
#include "param.h"
#include "scope.h"
#include "statement.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern int parse_static_expr( char* str, func_unit* funit, int lineno, bool no_genvars );

extern db**         db_list;
extern unsigned int curr_db;
extern char         user_msg[USER_MSG_LENGTH];
extern bool         debug_mode;
extern func_unit*   curr_funit;
extern int          curr_sig_id;


/*!
 Creates a user-readable version of the specified generate item and stores it in
 the specified string.
*/
static void gen_item_stringify(
  gen_item*    gi,      /*!< Pointer to generate item to stringify */
  char*        str,     /*!< Pointer to string to store data into */
  unsigned int str_len  /*!< Number of available characters in the str string */
) { PROFILE(GEN_ITEM_STRINGIFY);

  char* tmp;  /* Temporary string */

  assert( str_len > 0 );

  if( gi != NULL ) {

    unsigned int rv;

    /* Allocate some memory in the tmp string */
    tmp = (char*)malloc_safe_nolimit( str_len );

    rv = snprintf( str, str_len, "%p, suppl: %x", gi, gi->suppl.all );
    assert( rv < str_len );

    switch( gi->suppl.part.type ) {
      case GI_TYPE_EXPR :
        rv = snprintf( tmp, str_len, ", EXPR, %s", expression_string( gi->elem.expr ) );
        assert( rv < str_len );
        break;
      case GI_TYPE_SIG :
        rv = snprintf( tmp, str_len, ", SIG, name: %s", obf_sig( gi->elem.sig->name ) );
        assert( rv < str_len );
        break;
      case GI_TYPE_STMT :
        rv = snprintf( tmp, str_len, ", STMT, id: %d, line: %u", gi->elem.stmt->exp->id, gi->elem.stmt->exp->line );
        assert( rv < str_len );
        break;
      case GI_TYPE_INST :
        rv = snprintf( tmp, str_len, ", INST, name: %s", obf_inst( gi->elem.inst->name ) );
        assert( rv < str_len );
        break;
      case GI_TYPE_TFN :
        rv = snprintf( tmp, str_len, ", TFN, name: %s, type: %s", obf_inst( gi->elem.inst->name ), get_funit_type( gi->elem.inst->funit->suppl.part.type ) );
        assert( rv < str_len );
        break;
      case GI_TYPE_BIND :
        rv = snprintf( tmp, str_len, ", BIND, %s", expression_string( gi->elem.expr ) );
        assert( rv < str_len );
        break;
      default :
        strcpy( tmp, "UNKNOWN!\n" );
        break;
    }
    strcat( str, tmp );

    rv = snprintf( tmp, str_len, ", next_true: %p, next_false: %p", gi->next_true, gi->next_false );
    assert( rv < str_len );
    strcat( str, tmp );

    if( gi->varname != NULL ) {
      rv = snprintf( tmp, str_len, ", varname: %s", obf_sig( gi->varname ) );
      assert( rv < str_len );
      strcat( str, tmp );
    }

    /* Deallocate the temporary string memory */
    free_safe( tmp, str_len );

  } else {

    str[0] = '\0';

  }

  PROFILE_END;

}

/*!
 Displays the contents of the specified generate item to standard output (used for debugging purposes).
*/
void gen_item_display(
  gen_item* gi  /*!< Pointer to generate item to display */
) { PROFILE(GEN_ITEM_DISPLAY);

  char str[4096];  /* String to store data into */

  gen_item_stringify( gi, str, 4096 );

  printf( "  %s\n", str );

  PROFILE_END;

}

/*!
 Displays an entire generate block to standard output (used for debugging purposes).
*/
static void gen_item_display_block_helper(
  gen_item* root  /*!< Pointer to starting generate item to display */
) { PROFILE(GEN_ITEM_DISPLAY_BLOCK_HELPER);

  if( root != NULL ) {

    /* Display ourselves */
    gen_item_display( root );

    if( root->next_true == root->next_false ) {

      if( root->suppl.part.stop_true == 0 ) {
        gen_item_display_block_helper( root->next_true );
      }

    } else {

      if( root->suppl.part.stop_true == 0 ) {
        gen_item_display_block_helper( root->next_true );
      }

      if( root->suppl.part.stop_false == 0 ) {
        gen_item_display_block_helper( root->next_false );
      }

    }

  }

  PROFILE_END;

}

/*!
 Displays an entire generate block to standard output (used for debugging purposes).
*/
void gen_item_display_block(
  gen_item* root  /*!< Pointer to starting generate item to display */
) { PROFILE(GEN_ITEM_DISPLAY_BLOCK);

  printf( "Generate block:\n" );

  gen_item_display_block_helper( root );

  PROFILE_END;

}

/*!
 \return Returns TRUE if the two specified generate items are equivalent; otherwise,
         returns FALSE.
*/
static bool gen_item_compare(
  gen_item* gi1,  /*!< Pointer to first generate item to compare */
  gen_item* gi2   /*!< Pointer to second generate item to compare */
) { PROFILE(GEN_ITEM_COMPARE);

  bool retval = FALSE;  /* Return value for this function */

  if( (gi1 != NULL) && (gi2 != NULL) && (gi1->suppl.part.type == gi2->suppl.part.type) ) {

    switch( gi1->suppl.part.type ) {
      case GI_TYPE_EXPR :  retval = (gi1->elem.expr->id == gi2->elem.expr->id) ? TRUE : FALSE;  break;
      case GI_TYPE_SIG  :  retval = scope_compare( gi1->elem.sig->name, gi2->elem.sig->name );  break;
      case GI_TYPE_STMT :  retval = (gi1->elem.stmt->exp->id == gi2->elem.stmt->exp->id) ? TRUE : FALSE;  break;
      case GI_TYPE_INST :
      case GI_TYPE_TFN  :  retval = (strcmp( gi1->elem.inst->name, gi2->elem.inst->name ) == 0) ? TRUE : FALSE;  break;
      case GI_TYPE_BIND :  retval = ((gi1->elem.expr->id == gi2->elem.expr->id) &&
                                     (strcmp( gi1->varname, gi2->varname ) == 0)) ? TRUE : FALSE;  break;
      default           :  break;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the generate item that matches the given generate item
         in the given generate item block.  Returns NULL if it was not able to find
         a matching item.

 Recursively traverses the specified generate item block searching for a generate item
 that matches the specified generate item.
*/
gen_item* gen_item_find(
  gen_item* root,  /*!< Pointer to root of generate item block to search in */
  gen_item* gi     /*!< Pointer to generate item to search for */
) { PROFILE(GEN_ITEM_FIND);

  gen_item* found = NULL;  /* Return value for this function */

  assert( gi != NULL );

  if( root != NULL ) {

    if( gen_item_compare( root, gi ) ) {

      found = root;

    } else {

      /* If both true and false paths lead to same item, just traverse the true path */
      if( root->next_true == root->next_false ) {

        if( root->suppl.part.stop_true == 0 ) {
          found = gen_item_find( root->next_true, gi );
        }

      /* Otherwise, traverse both true and false paths */
      } else if( (root->suppl.part.stop_true == 0) && ((found = gen_item_find( root->next_true, gi )) == NULL) ) {

        if( root->suppl.part.stop_false == 0 ) {
          found = gen_item_find( root->next_false, gi );
        }

      }

    }

  }

  PROFILE_END;

  return( found );

}

/*!
 Removes the given generate item if it contains an expressions that calls a statement.
*/
void gen_item_remove_if_contains_expr_calling_stmt(
  gen_item*  gi,   /*!< Pointer to generate item list to search */
  statement* stmt  /*!< Pointer to statement to search for */
) { PROFILE(GEN_ITEM_REMOVE_IF_CONTAINS_EXPR_CALLING_STMT);

  if( gi != NULL ) {

    if( gi->suppl.part.type == GI_TYPE_STMT ) {

      if( statement_contains_expr_calling_stmt( gi->elem.stmt, stmt ) ) {
        gi->suppl.part.removed = 1;
      }

    } else {

      /* If both true and false paths lead to same item, just traverse the true path */
      if( gi->next_true == gi->next_false ) {

        if( gi->suppl.part.stop_true == 0 ) {
          gen_item_remove_if_contains_expr_calling_stmt( gi->next_true, stmt );
        }

      /* Otherwise, traverse both true and false paths */
      } else {

        if( gi->suppl.part.stop_true == 0 ) {
          gen_item_remove_if_contains_expr_calling_stmt( gi->next_true, stmt );
        }

        if( gi->suppl.part.stop_false == 0 ) {
          gen_item_remove_if_contains_expr_calling_stmt( gi->next_false, stmt );
        }

      }

    }

  }

  PROFILE_END;

}

/*!
 Searches the given variable name for a generate variable.  If one is found, pre is
 set to point to the string preceding the generate variable, genvar is set to point
 to the beginning of the generate variable, and post is set to point to the string
 succeeding the ']'.
*/
static void gen_item_get_genvar(
            char*  varname,  /*!< Variable name to search */
  /*@out@*/ char** pre,      /*!< Reference pointer to string preceding the generate variable */
  /*@out@*/ char** genvar,   /*!< Reference pointer to found generate variable name */
  /*@out@*/ char** post      /*!< Reference pointer to string succeeding the generate variable */
) { PROFILE(GEN_ITEM_GET_GENVAR);

  int i = 0;  /* Loop iterator */

  /* Initialize pointers */
  *pre    = varname;
  *genvar = NULL;
  *post   = NULL;

  /* Iterate through the variable name until we either reach the end of the string or until we have found a genvar */
  while( (varname[i] != '\0') && (*genvar == NULL) ) {

    /* Iterate through the varname until we see either a \, [ or terminating character */
    while( (varname[i] != '\\') && (varname[i] != '[') && (varname[i] != '\0') ) {
      i++;
    }
 
    /* If we saw a \ character, keep going until we see whitespace */
    if( varname[i] == '\\' ) {
      while( (varname[i] != ' ') && (varname[i] != '\n') && (varname[i] != '\t') && (varname[i] != '\r') ) {
        i++;
      }

    /* If we saw a [, get the genvar name, stripping all whitespace from the name */
    } else if( varname[i] == '[' ) {

      varname[i] = '\0';
      i++;
      while( (varname[i] == ' ') || (varname[i] == '\n') || (varname[i] == '\t') || (varname[i] == '\r') ) {
        i++;
      }
      *genvar = (varname + i);
      while( (varname[i] != ' ') && (varname[i] != '\n') && (varname[i] != '\t') && (varname[i] != '\r') && (varname[i] != ']') ) {
        i++;
      }
      if( varname[i] != ']' ) {
        varname[i] = '\0';
        i++;
        while( varname[i] != ']' ) {
          i++;
        }
      } else {
        varname[i] = '\0';
      }
      i++;
      *post = (varname + i);

    }

  }

  PROFILE_END;

}

/*!
 \param name  Variable name to check

 \return Returns TRUE if the specified variable name contains a generate variable within it; otherwise,
         returns FALSE.

 This function is called by db_create_expression() just before it places its signal name in the binding
 list.  If the specified signal name contains a generate variable, we need to create a generate item
 binding element so that we can properly substitute the generate variable name with its current value.
*/
bool gen_item_varname_contains_genvar( char* name ) { PROFILE(GEN_ITEM_VARNAME_CONTAINS_GENVAR);

  bool  retval = FALSE;  /* Return value for this function */
  char* pre;             /* String prior to the generate variable */
  char* genvar;          /* Generate variable */
  char* post;            /* String after the generate variable */
  char* tmpname;         /* Copy of the given name */

  /* Allocate memory */
  tmpname = strdup_safe( name );
  
  /* Find the first generate variable */
  gen_item_get_genvar( tmpname, &pre, &genvar, &post );

  if( genvar != NULL ) {
    retval = TRUE;
  }

  /* Deallocate memory */
  free_safe( tmpname, (strlen( name ) + 1)  );

  return( retval );

}

/*!
 \param name        Name of signal that we possibly need to convert if it contains generate variable(s)
 \param funit       Pointer to current functional unit
 \param line        Line number in which the signal's expression exists
 \param no_genvars  If set to TRUE, we need to make sure that we do not see a generate variable in the
                    generate hierarchical expression.

 \return Returns allocated string containing the signal name with embedded generate variables evaluated

 \throws anonymous parse_static_expr Throw

 Iterates through the given name, substituting any found generate variables with their current value.
*/
char* gen_item_calc_signal_name(
  const char* name,
  func_unit*  funit,
  int         line,
  bool        no_genvars
) { PROFILE(GEN_ITEM_CALC_SIGNAL_NAME);

  char* new_name = NULL;  /* Return value of this function */
  char* tmpname;          /* Temporary name of current part of variable */
  char* pre;              /* String prior to the generate variable */
  char* genvar;           /* Generate variable */
  char* post;             /* String after the generate variable */
  char  intstr[20];       /* String containing an integer value */
  char* ptr;              /* Pointer to allocated memory for name */

  /* Allocate memory */
  tmpname  = strdup_safe( name );
  ptr      = tmpname;
  new_name = strdup_safe( "" );

  Try {

    do {
      gen_item_get_genvar( tmpname, &pre, &genvar, &post );
      if( genvar != NULL ) {
        unsigned int rv = snprintf( intstr, 20, "%d", parse_static_expr( genvar, funit, line, no_genvars ) );
        assert( rv < 20 );
        new_name = (char*)realloc_safe( new_name, (strlen( new_name ) + 1), (strlen( new_name ) + strlen( pre ) + strlen( intstr ) + 3) );
        strncat( new_name, pre, strlen( pre ) );
        strncat( new_name, "[", 1 );
        strncat( new_name, intstr, strlen( intstr ) );
        strncat( new_name, "]", 1 );
        tmpname = post;
      } else {
        new_name = (char*)realloc_safe( new_name, (strlen( new_name ) + 1), (strlen( new_name ) + strlen( pre ) + 1) );
        strncat( new_name, pre, strlen( pre ) );
      }
    } while( genvar != NULL );

  } Catch_anonymous {
    free_safe( new_name, (strlen( new_name ) + 1) );
    free_safe( ptr, (strlen( name ) + 1) );
    Throw 0;
  }

  /* Deallocate memory */
  free_safe( ptr, (strlen( name ) + 1) );

  /* Make sure that the new_name is set to something */
  assert( new_name != NULL );

  return( new_name );

}

/*!
 \param expr  Pointer to root expression to create (this is an expression from a FOR, IF or CASE statement)

 \return Returns a pointer to created generate item.

 Creates a new generate item for an expression.
*/
gen_item* gen_item_create_expr(
  expression* expr
) { PROFILE(GEN_ITEM_CREATE_EXPR);

  gen_item* gi;

  /* Create the generate item for an expression */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ) );
  gi->elem.expr       = expr;
  gi->suppl.all       = 0;
  gi->suppl.part.type = GI_TYPE_EXPR;
  gi->varname         = NULL;
  gi->next_true       = NULL;
  gi->next_false      = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char         str[USER_MSG_LENGTH];
    unsigned int rv;
    gen_item_stringify( gi, str, USER_MSG_LENGTH );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_expr, %s", str );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param sig  Pointer to signal to create a generate item for (wire/reg instantions)

 \return Returns a pointer to created generate item.

 Creates a new generate item for a signal.
*/
gen_item* gen_item_create_sig(
  vsignal* sig
) { PROFILE(GEN_ITEM_CREATE_SIG);

  gen_item* gi;

  /* Create the generate item for a signal */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ) );
  gi->elem.sig        = sig;
  gi->suppl.all       = 0;
  gi->suppl.part.type = GI_TYPE_SIG;
  gi->varname         = NULL;
  gi->next_true       = NULL;
  gi->next_false      = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char         str[USER_MSG_LENGTH];
    unsigned int rv;
    gen_item_stringify( gi, str, USER_MSG_LENGTH );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_sig, %s", str );
    assert( rv < USER_MSG_LENGTH );
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
gen_item* gen_item_create_stmt(
  statement* stmt
) { PROFILE(GEN_ITEM_CREATE_STMT);

  gen_item* gi;

  /* Create the generate item for a statement */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ) );
  gi->elem.stmt       = stmt;
  gi->suppl.all       = 0;
  gi->suppl.part.type = GI_TYPE_STMT;
  gi->varname         = NULL;
  gi->next_true       = NULL;
  gi->next_false      = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char         str[USER_MSG_LENGTH];
    unsigned int rv;
    gen_item_stringify( gi, str, USER_MSG_LENGTH );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_stmt, %s", str );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param inst  Pointer to instance to create a generate item for (instantiations)

 \return Returns a pointer to create generate item.

 Create a new generate item for an instance.
*/
gen_item* gen_item_create_inst(
  funit_inst* inst
) { PROFILE(GEN_ITEM_CREATE_INST);

  gen_item* gi;

  /* Create the generate item for an instance */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ) );
  gi->elem.inst       = inst;
  gi->suppl.all       = 0;
  gi->suppl.part.type = GI_TYPE_INST;
  gi->varname         = NULL;
  gi->next_true       = NULL;
  gi->next_false      = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char         str[USER_MSG_LENGTH];
    unsigned int rv;
    gen_item_stringify( gi, str, USER_MSG_LENGTH );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_inst, %s", str );
    assert( rv < USER_MSG_LENGTH );
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
gen_item* gen_item_create_tfn(
  funit_inst* inst
) { PROFILE(GEN_ITEM_CREATE_TFN);

  gen_item* gi;

  /* Create the generate item for a namespace */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ) );
  gi->elem.inst       = inst;
  gi->suppl.all       = 0;
  gi->suppl.part.type = GI_TYPE_TFN;
  gi->varname         = NULL;
  gi->next_true       = NULL;
  gi->next_false      = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char str[USER_MSG_LENGTH];
    unsigned int rv;
    gen_item_stringify( gi, str, USER_MSG_LENGTH );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_tfn, %s", str );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \param name  Name of signal to bind to
 \param expr  Pointer to expression to bind signal to

 \return Returns a pointer to create generate item.

 Create a new generate item for a namespace.
*/
gen_item* gen_item_create_bind(
  const char* name,
  expression* expr
) { PROFILE(GEN_ITEM_CREATE_BIND);

  gen_item* gi;

  /* Create the generate item for a namespace */
  gi = (gen_item*)malloc_safe( sizeof( gen_item ) );
  gi->elem.expr       = expr;
  gi->suppl.all       = 0;
  gi->suppl.part.type = GI_TYPE_BIND;
  gi->varname         = strdup_safe( name );
  gi->next_true       = NULL;
  gi->next_false      = NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char         str[USER_MSG_LENGTH];
    unsigned int rv;
    gen_item_stringify( gi, str, USER_MSG_LENGTH );
    rv = snprintf( user_msg, USER_MSG_LENGTH, "In gen_item_create_bind, %s", str );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  return( gi );

}

/*!
 \throws anonymous vsignal_create_vec statement_size_elements gen_item_resize_stmts_and_sigs gen_item_resize_stmts_and_sigs gen_item_resize_stmts_and_sigs

 Recursively iterates the the specified generate item block, resizing all statements
 within that block.
*/
void gen_item_resize_stmts_and_sigs(
  gen_item*  gi,    /*!< Pointer to generate item block to check */
  func_unit* funit  /*!< Pointer to functional unit that contains this generate item */
) { PROFILE(GEN_ITEM_RESIZE_STMTS_AND_SIGS);

  if( gi != NULL ) {

    /* Resize our statement, if we are one */
    if( gi->suppl.part.type == GI_TYPE_STMT ) {
      statement_size_elements( gi->elem.stmt, funit );
    } else if( gi->suppl.part.type == GI_TYPE_SIG ) {
      vsignal_create_vec( gi->elem.sig );
    }

    /* Go to the next generate item */
    if( gi->next_true == gi->next_false ) {
      if( gi->suppl.part.stop_true == 0 ) {
        gen_item_resize_stmts_and_sigs( gi->next_true, funit );
      }
    } else {
      if( gi->suppl.part.stop_false == 0 ) {
        gen_item_resize_stmts_and_sigs( gi->next_false, funit );
      }
      if( gi->suppl.part.stop_true == 0 ) {
        gen_item_resize_stmts_and_sigs( gi->next_true, funit );
      }
    }

  }

  PROFILE_END;

}

/*!
 \throws anonymous statement_assign_expr_ids

 Assigns unique expression and signal IDs to each expression in the tree given for a generated statement
 or for the given signal.
*/
void gen_item_assign_ids(
  gen_item*  gi,    /*!< Pointer to generate item to check and assign expression IDs for */
  func_unit* funit  /*!< Pointer to functional unit containing this generate item */
) { PROFILE(GEN_ITEM_ASSIGN_IDS);

  if( gi->suppl.part.removed == 0 ) {

    /* Assign expression IDs if this is a statement */
    if( gi->suppl.part.type == GI_TYPE_STMT ) {
      statement_assign_expr_ids( gi->elem.stmt, funit );
    } else if( gi->suppl.part.type == GI_TYPE_SIG ) {
      gi->elem.sig->id = curr_sig_id;
      curr_sig_id++;
    }

  }

  PROFILE_END;

}

/*!
 Checks the given generate item against the supplied type.  If they match,
 outputs the given generate item to the specified output file.  If they do
 not match, nothing is done.
*/
void gen_item_db_write(
  gen_item* gi,    /*!< Pointer to current generate item to test and output */
  uint32    type,  /*!< Specifies the type of the generate item to output */
  FILE*     ofile  /*!< Output file to display generate item to */
) { PROFILE(GEN_ITEM_DB_WRITE);

  /* If the types match, output based on type */
  if( (gi->suppl.part.type == type) && (gi->suppl.part.removed == 0) ) {

    switch( type ) {
      case GI_TYPE_SIG :
        vsignal_db_write( gi->elem.sig, ofile );
        break;
      case GI_TYPE_STMT :
        statement_db_write_tree( gi->elem.stmt, ofile );
        break;
      default :  /* Should never be called */
        assert( (type == GI_TYPE_SIG) || (type == GI_TYPE_STMT) );
        break;
    }

  }

  PROFILE_END;

}

/*!
 Outputs all expressions for the statement contained in the specified generate item.
*/
void gen_item_db_write_expr_tree(
  gen_item* gi,    /*!< Pointer to current generate item to test and output */
  FILE*     ofile  /*!< Output file to display generate item to */
) { PROFILE(GEN_ITEM_DB_WRITE_EXPR_TREE);

  /* Only do this for statements */
  if( (gi->suppl.part.type == GI_TYPE_STMT) && (gi->suppl.part.removed == 0) ) {

    statement_db_write_expr_tree( gi->elem.stmt, ofile );

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the connection was successful; otherwise, returns FALSE.
*/
bool gen_item_connect(
  gen_item* gi1,          /*!< Pointer to generate item block to connect to gi2 */
  gen_item* gi2,          /*!< Pointer to generate item to connect to gi1 */
  int       conn_id,      /*!< Connection ID */
  bool      stop_on_null  /*!< Sets stop bit(s) if a NULL next_true/next_false pointer is found */
) { PROFILE(GEN_ITEM_CONNECT);

  bool retval = FALSE;  /* Return value for this function */

  /* Set the connection ID */
  gi1->suppl.part.conn_id = conn_id;

  /* If both paths go to the same destination, only parse one path */
  if( gi1->next_true == gi1->next_false ) {

    /* If the TRUE path is NULL, connect it to the new statement */
    if( gi1->next_true == NULL ) {
      gi1->next_true  = gi2;
      gi1->next_false = gi2;
      if( (gi1->next_true->suppl.part.conn_id == conn_id) || stop_on_null ) {
        gi1->suppl.part.stop_true  = 1;
        gi1->suppl.part.stop_false = 1;
      }
      retval = TRUE;
    } else if( gi1->next_true->suppl.part.conn_id == conn_id ) {
      gi1->suppl.part.stop_true  = 1;
      gi1->suppl.part.stop_false = 1;

    /* If the TRUE path leads to a loop/merge, stop traversing */
    } else if( gi1->next_true != gi2 ) {
      retval |= gen_item_connect( gi1->next_true, gi2, conn_id, stop_on_null );
    }

  } else {

    /* Traverse FALSE path */
    if( gi1->next_false == NULL ) {
      gi1->next_false = gi2;
      if( (gi1->next_false->suppl.part.conn_id == conn_id) || stop_on_null ) {
        gi1->suppl.part.stop_false = 1;
      } else {
        gi1->next_false->suppl.part.conn_id = conn_id;
      }
      retval = TRUE;
    } else if( gi1->next_false->suppl.part.conn_id == conn_id ) {
      gi1->suppl.part.stop_false = 1;
    } else if( gi1->next_false != gi2 ) {
      retval |= gen_item_connect( gi1->next_false, gi2, conn_id, stop_on_null );
    }

    /* Traverse TRUE path */
    if( gi1->next_true == NULL ) {
      gi1->next_true = gi2;
      if( (gi1->next_true->suppl.part.conn_id == conn_id) || stop_on_null ) {
        gi1->suppl.part.stop_true = 1;
      }
      retval = TRUE;
    } else if( gi1->next_true->suppl.part.conn_id == conn_id ) {
      gi1->suppl.part.stop_true = 1;
    } else if( (gi1->next_true != gi2) && (gi1->suppl.part.type != GI_TYPE_TFN) && (gi1->varname == NULL) ) {
      retval |= gen_item_connect( gi1->next_true, gi2, conn_id, TRUE );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \throws anonymous gen_item_calc_signal_name Throw expression_resize expression_operate_recursively gen_item_resolve
                   gen_item_resolve gen_item_resolve gen_item_resolve gen_item_resolve gen_item_resolve gen_item_resolve gen_item_resolve

 Recursively iterates through the entire generate block specified by gi, resolving all generate items
 within it.  This is called by the generate_resolve function (in the middle of the binding process) and
 by the funit_size_elements function (just prior to outputting this instance to the CDD file).
*/
static void gen_item_resolve(
  gen_item*   gi,   /*!< Pointer to current generate item to resolve */
  funit_inst* inst  /*!< Pointer to instance to store results to */
) { PROFILE(GEN_ITEM_RESOLVE);

  funit_inst* child;    /* Pointer to child instance of this instance to resolve */
  char*       varname;  /* Pointer to new, substituted name (used for BIND types) */

  if( (gi != NULL) && (inst != NULL) ) {

#ifdef DEBUG_MODE 
    if( debug_mode ) {
      char*        str = (char*)malloc_safe( USER_MSG_LENGTH );
      unsigned int rv;
      gen_item_stringify( gi, str, USER_MSG_LENGTH );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Resolving generate item, %s for inst: %s", str, obf_inst( inst->name ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
      free_safe( str, USER_MSG_LENGTH );
    }
#endif

    /* Specify that this generate item has been resolved */
    gi->suppl.part.resolved = 1;

    switch( gi->suppl.part.type ) {
  
      case GI_TYPE_EXPR :
        /* Recursively resize the expression tree if we have not already done this */
        if( gi->elem.expr->exec_num == 0 ) {
          expression_resize( gi->elem.expr, inst->funit, TRUE, FALSE );
        }
        expression_operate_recursively( gi->elem.expr, inst->funit, FALSE );
        if( ESUPPL_IS_TRUE( gi->elem.expr->suppl ) ) {
          gen_item_resolve( gi->next_true, inst );
        } else {
          gen_item_resolve( gi->next_false, inst );
        }
        break;

      case GI_TYPE_SIG :
        gitem_link_add( gen_item_create_sig( gi->elem.sig ), &(inst->gitem_head), &(inst->gitem_tail) );
        if( sig_link_find( gi->elem.sig->name, inst->funit->sigs, inst->funit->sig_size ) == NULL ) {
          sig_link_add( gi->elem.sig, FALSE, &(inst->funit->sigs), &(inst->funit->sig_size), &(inst->funit->sig_no_rm_index) );
        }
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_STMT :
        gitem_link_add( gen_item_create_stmt( gi->elem.stmt ), &(inst->gitem_head), &(inst->gitem_tail) );
        if( stmt_link_find( gi->elem.stmt->exp->id, inst->funit->stmt_head ) == NULL ) {
          statement_add_to_stmt_link( gi->elem.stmt, &(inst->funit->stmt_head), &(inst->funit->stmt_tail) );
          // stmt_link_add( gi->elem.stmt, FALSE, &(inst->funit->stmt_head), &(inst->funit->stmt_tail) );
        }
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_INST :
        {
          funit_inst* tinst;
          if( gi->elem.inst->funit->suppl.part.type == FUNIT_MODULE ) {
            funit_link* found_funit_link;
            if( ((found_funit_link = funit_link_find( gi->elem.inst->funit->name, gi->elem.inst->funit->suppl.part.type, db_list[curr_db]->funit_head )) != NULL) &&
                (gi->elem.inst->funit != found_funit_link->funit) ) {
              /* Make sure that any instances in the tree that point to the functional unit being replaced are pointing to the new functional unit */
              int ignore = 0;
              while( (tinst = inst_link_find_by_funit( gi->elem.inst->funit, db_list[curr_db]->inst_head, &ignore )) != NULL ) {
                tinst->funit = found_funit_link->funit;
              }
              funit_dealloc( gi->elem.inst->funit );
              gi->elem.inst->funit = found_funit_link->funit;
            }
          }
          if( (tinst = instance_copy( gi->elem.inst, inst, gi->elem.inst->name, gi->elem.inst->ppfline, gi->elem.inst->fcol, gi->elem.inst->range, FALSE )) != NULL ) {
            param_resolve_inst( tinst );
          }
          gen_item_resolve( gi->next_true, inst );
        }
        break;

      case GI_TYPE_TFN :
        {
          char* inst_name = (char*)malloc_safe( 4096 );
          if( gi->varname != NULL ) {
            vsignal*     genvar;
            func_unit*   found_funit;
            unsigned int rv;
            if( !scope_find_signal( gi->varname, inst->funit, &genvar, &found_funit, 0 ) ) {
              rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find variable %s in module %s",
                             obf_sig( gi->varname ), obf_funit( inst->funit->name ) );
              assert( rv < USER_MSG_LENGTH );
              print_output( user_msg, FATAL, __FILE__, __LINE__ );
              Throw 0;
            }
            rv = snprintf( inst_name, 4096, "%s[%d]", gi->elem.inst->name, vector_to_int( genvar->value ) );
            assert( rv < 4096 );
            (void)instance_parse_add( &inst, inst->funit, gi->elem.inst->funit, inst_name, gi->elem.inst->ppfline, gi->elem.inst->fcol, NULL, FALSE, TRUE, FALSE, TRUE );
            rv = snprintf( inst_name, 4096, "%s.%s[%d]", inst->name, gi->elem.inst->name, vector_to_int( genvar->value ) );
            assert( rv < 4096 );
            if( (child = instance_find_scope( inst, inst_name, TRUE )) != NULL ) {
              inst_parm_add_genvar( genvar, child );
            }
          } else {
            char         inst_name[4096];
            unsigned int rv;
            (void)instance_parse_add( &inst, inst->funit, gi->elem.inst->funit, gi->elem.inst->name, gi->elem.inst->ppfline, gi->elem.inst->fcol, NULL, FALSE, TRUE, FALSE, TRUE );
            rv = snprintf( inst_name, 4096, "%s.%s", inst->name, gi->elem.inst->name );
            assert( rv < 4096 );
            child = instance_find_scope( inst, inst_name, TRUE );
          }
          free_safe( inst_name, 4096 );
          if( child != NULL ) {
            param_resolve_inst( child );
            if( child->funit->suppl.part.type != FUNIT_MODULE ) {
              func_unit* parent_mod = funit_get_curr_module( child->funit );
              funit_link_add( child->funit, &(parent_mod->tf_head), &(parent_mod->tf_tail) );
              child->funit->parent = inst->funit;
            }
          }
          gen_item_resolve( gi->next_true, child );
          gen_item_resolve( gi->next_false, inst );
        }
        break;

      case GI_TYPE_BIND :
        varname = gen_item_calc_signal_name( gi->varname, inst->funit, gi->elem.expr->line, FALSE );
        switch( gi->elem.expr->op ) {
          case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    varname, gi->elem.expr, inst->funit, FALSE );  break;
          case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        varname, gi->elem.expr, inst->funit, FALSE );  break;
          case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, varname, gi->elem.expr, inst->funit, FALSE );  break;
          case EXP_OP_DISABLE   :  bind_add( 1,                 varname, gi->elem.expr, inst->funit, FALSE );  break;
          default               :  bind_add( 0,                 varname, gi->elem.expr, inst->funit, FALSE );  break;
        }
        gitem_link_add( gen_item_create_bind( varname, gi->elem.expr ), &(inst->gitem_head), &(inst->gitem_tail) );
        free_safe( varname, (strlen( varname ) + 1) ); 
        gen_item_resolve( gi->next_true, inst );
        break;

      default :
        assert( (gi->suppl.part.type == GI_TYPE_EXPR) || (gi->suppl.part.type == GI_TYPE_SIG) ||
                (gi->suppl.part.type == GI_TYPE_STMT) || (gi->suppl.part.type == GI_TYPE_INST) ||
                (gi->suppl.part.type == GI_TYPE_TFN)  || (gi->suppl.part.type == GI_TYPE_BIND) );
        break;

    }

  }

  PROFILE_END;

}

/*!
 Updates the specified expression name to be that of the generate item name
 if the current generate item is a BIND type.
*/
void gen_item_bind(
  gen_item* gi  /*!< Pointer to generate item to examine */
) { PROFILE(GEN_ITEM_BIND);

  if( gi->suppl.part.type == GI_TYPE_BIND ) {

    /* Remove the current name */
    free_safe( gi->elem.expr->name, (strlen( gi->elem.expr->name ) + 1) );

    /* Assign the new name */
    gi->elem.expr->name = strdup_safe( gi->varname );

  }

  PROFILE_END;

}

/*!
 \throws anonymous generate_resolve gen_item_resolve

 Resolves all generate items in the given instance.  This is called at a specific point
 in the binding process.
*/
void generate_resolve_inst(
  funit_inst* inst  /*!< Pointer to current instance to resolve for */
) { PROFILE(GENERATE_RESOLVE_INST);

  if( inst != NULL ) {

    /* Resolve ourself */
    if( inst->funit != NULL ) {
      gitem_link* curr_gi = inst->funit->gitem_head;
      while( curr_gi != NULL ) {
        gen_item_resolve( curr_gi->gi, inst );
        curr_gi = curr_gi->next;
      }
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if we found at least one match; otherwise, returns FALSE.
*/
static bool generate_remove_stmt_helper(
  funit_inst* root,  /*!< Pointer to root instance to traverse */
  statement*  stmt   /*!< Pointer to statement to find and remove */
) { PROFILE(GENERATE_REMOVE_STMT_HELPER);

  bool        retval   = FALSE;  /* Return value for this function */
  funit_inst* curr_child;        /* Pointer to current child to search */
  gitem_link* gil;               /* Pointer to generate item link */

  /* Remove the generate item from the current instance if it exists there */
  gil = root->gitem_head;
  while( gil != NULL ) {
    if( (gil->gi->suppl.part.type == GI_TYPE_STMT) && (statement_find_statement( gil->gi->elem.stmt, stmt->exp->id ) != NULL) ) {
      gil->gi->suppl.part.removed = 1;
      retval = TRUE;
    }
    gil = gil->next;
  }

  /* Search child instances */
  curr_child = root->child_head;
  while( curr_child != NULL ) {
    retval |= generate_remove_stmt_helper( curr_child, stmt );
    curr_child = curr_child->next;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if we found at least one match; otherwise, returns FALSE.

 Iterates through the entire instance tree finding and "removing" all statement generate items
 that match the given statement ID.  This will get called by the stmt_blk_remove() function
 when a statement has been found that does not exist in a functional unit.
*/
bool generate_remove_stmt(
  statement* stmt  /*!< Statement to set "remove" bit on */
) { PROFILE(GENERATE_REMOVE_STMT);

  bool       retval = FALSE;  /* Return value for this function */
  inst_link* instl;           /* Pointer to current instance list to parse */

  /* Search for the generate item in the instance lists */
  instl = db_list[curr_db]->inst_head;
  while( instl != NULL ) {
    retval |= generate_remove_stmt_helper( instl->inst, stmt );
    instl = instl->next;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns pointer to found statement.  If no statement was found, returns NULL.
*/
static statement* generate_find_stmt_by_position_helper(
  gen_item*    gi,          /*!< Pointer to current generate item to examine */
  unsigned int first_line,  /*!< First line of statement to search for */
  unsigned int first_col    /*!< First column of statement to search for */
) { PROFILE(GENERATE_FIND_STMT_BY_POSITION_HELPER);

  statement* stmt = NULL;

  if( gi != NULL ) {

    if( (gi->suppl.part.type == GI_TYPE_STMT) && (gi->elem.stmt->exp->line <= first_line) ) {
      stmt = statement_find_statement_by_position( gi->elem.stmt, first_line, first_col );
    }

    if( stmt == NULL ) {
      if( gi->next_true == gi->next_false ) {
        if( gi->suppl.part.stop_true == 0 ) {
          stmt = generate_find_stmt_by_position_helper( gi->next_true, first_line, first_col );
        }
      } else {
        if( gi->suppl.part.stop_true == 0 ) {
          stmt = generate_find_stmt_by_position_helper( gi->next_true, first_line, first_col );
          if( (stmt == NULL) && (gi->suppl.part.stop_false == 0) ) {
            stmt = generate_find_stmt_by_position_helper( gi->next_false, first_line, first_col );
          }
        }
      }
    }

  }

  PROFILE_END;

  return( stmt );

}

/*!
 \return Returns pointer to found statement

 Searches the generate block of the given functional unit (module) for a statement that matches the
 given file positional information.  If none is found, returns NULL.
*/
statement* generate_find_stmt_by_position(
  func_unit*   funit,       /*!< Pointer to module that contains the generate item block */
  unsigned int first_line,  /*!< First line of statement to search for */
  unsigned int first_col    /*!< First column of statement to search for */
) { PROFILE(GENERATE_FIND_STMT_BY_POSITION);

  gitem_link* gil  = funit->gitem_head;
  statement*  stmt = NULL;

  while( (gil != NULL) && ((stmt = generate_find_stmt_by_position_helper( gil->gi, first_line, first_col )) == NULL) ) {
    gil = gil->next;
  }

  PROFILE_END;

  return( stmt );

}

/*!
 \return Returns pointer to found statement.  If no statement was found, returns NULL.
*/
static func_unit* generate_find_tfn_by_position_helper(
  gen_item*    gi,          /*!< Pointer to current generate item to examine */
  unsigned int first_line,  /*!< First line of statement to search for */
  unsigned int first_col    /*!< First column of statement to search for */
) { PROFILE(GENERATE_FIND_TFN_BY_POSITION_HELPER);

  func_unit* funit = NULL;

  if( gi != NULL ) {

    if( (gi->suppl.part.type == GI_TYPE_TFN) &&
        (gi->elem.inst->funit->start_line == first_line) &&
        (gi->elem.inst->funit->start_col  == first_col) ) {
      funit = gi->elem.inst->funit;
    }

    if( funit == NULL ) {
      if( gi->next_true == gi->next_false ) {
        if( gi->suppl.part.stop_true == 0 ) {
          funit = generate_find_tfn_by_position_helper( gi->next_true, first_line, first_col );
        }
      } else {
        if( gi->suppl.part.stop_true == 0 ) {
          funit = generate_find_tfn_by_position_helper( gi->next_true, first_line, first_col );
          if( (funit == NULL) && (gi->suppl.part.stop_false == 0) ) {
            funit = generate_find_tfn_by_position_helper( gi->next_false, first_line, first_col );
          }
        }
      }
    }

  }

  PROFILE_END;

  return( funit );

}

/*!
 \return Returns pointer to found functional unit

 Searches the generate block of the given functional unit (module) for a task/function/namedblock that matches the
 given file positional information.  If none is found, returns NULL.
*/
func_unit* generate_find_tfn_by_position(
  func_unit*   funit,       /*!< Pointer to module that contains the generate item block */
  unsigned int first_line,  /*!< First line of statement to search for */
  unsigned int first_col    /*!< First column of statement to search for */
) { PROFILE(GENERATE_FIND_TFN_BY_POSITION);

  gitem_link* gil = funit->gitem_head;
  func_unit*  tfn = NULL;

  while( (gil != NULL) && ((tfn = generate_find_tfn_by_position_helper( gil->gi, first_line, first_col )) == NULL) ) {
    gil = gil->next;
  }

  PROFILE_END;

  return( tfn );

}

/*!
 Recursively deallocates the gen_item structure tree.
*/
void gen_item_dealloc(
  gen_item* gi,      /*!< Pointer to gen_item structure to deallocate */
  bool      rm_elem  /*!< If set to TRUE, removes the associated element */
) { PROFILE(GEN_ITEM_DEALLOC);

  if( gi != NULL ) {

    /* Deallocate children first */
    if( gi->next_true == gi->next_false ) {
      if( gi->suppl.part.stop_true == 0 ) {
        gen_item_dealloc( gi->next_true, rm_elem );
      }
    } else {
      if( gi->suppl.part.stop_false == 0 ) {
        gen_item_dealloc( gi->next_false, rm_elem );
      }
      if( gi->suppl.part.stop_true == 0 ) {
        gen_item_dealloc( gi->next_true, rm_elem );
      }
    }

    /* If we need to remove the current element, do so now */
    if( rm_elem ) {
      switch( gi->suppl.part.type ) {
        case GI_TYPE_EXPR :
          expression_dealloc( gi->elem.expr, FALSE );
          break;
        case GI_TYPE_SIG  :
          vsignal_dealloc( gi->elem.sig );
          break;
        case GI_TYPE_STMT :
          statement_dealloc_recursive( gi->elem.stmt, FALSE );
          break;
        case GI_TYPE_INST :
        case GI_TYPE_TFN  :
          instance_dealloc_tree( gi->elem.inst );
          break;
        case GI_TYPE_BIND :
          break;
        default           :  break;
      }
    }

    /* Remove the varname if necessary */
    free_safe( gi->varname, (strlen( gi->varname ) + 1) );

    /* Now deallocate ourselves */
    free_safe( gi, sizeof( gen_item ) );

  }

  PROFILE_END;

} 

