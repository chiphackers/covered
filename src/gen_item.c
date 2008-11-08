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
        rv = snprintf( tmp, str_len, ", STMT, id: %d, line: %d", gi->elem.stmt->exp->id, gi->elem.stmt->exp->line );
        assert( rv < str_len );
        break;
      case GI_TYPE_INST :
        rv = snprintf( tmp, str_len, ", INST, name: %s", obf_inst( gi->elem.inst->name ) );
        assert( rv < str_len );
        break;
      case GI_TYPE_TFN :
        rv = snprintf( tmp, str_len, ", TFN, name: %s, type: %s", obf_inst( gi->elem.inst->name ), get_funit_type( gi->elem.inst->funit->type ) );
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
    printf( "gen_item Throw A\n" );
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
  gen_item* gi1,     /*!< Pointer to generate item block to connect to gi2 */
  gen_item* gi2,     /*!< Pointer to generate item to connect to gi1 */
  int       conn_id  /*!< Connection ID */
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
      if( gi1->next_true->suppl.part.conn_id == conn_id ) {
        gi1->suppl.part.stop_true  = 1;
        gi1->suppl.part.stop_false = 1;
      }
      retval = TRUE;
    } else if( gi1->next_true->suppl.part.conn_id == conn_id ) {
      gi1->suppl.part.stop_true  = 1;
      gi1->suppl.part.stop_false = 1;

    /* If the TRUE path leads to a loop/merge, stop traversing */
    } else if( gi1->next_true != gi2 ) {
      retval |= gen_item_connect( gi1->next_true, gi2, conn_id );
    }

  } else {

    /* Traverse FALSE path */
    if( gi1->next_false == NULL ) {
      gi1->next_false = gi2;
      if( gi1->next_false->suppl.part.conn_id == conn_id ) {
        gi1->suppl.part.stop_false = 1;
      } else {
        gi1->next_false->suppl.part.conn_id = conn_id;
      }
      retval = TRUE;
    } else if( gi1->next_false->suppl.part.conn_id == conn_id ) {
      gi1->suppl.part.stop_false = 1;
    } else if( gi1->next_false != gi2 ) {
      retval |= gen_item_connect( gi1->next_false, gi2, conn_id );
    }

    /* Traverse TRUE path */
    if( gi1->next_true == NULL ) {
      gi1->next_true = gi2;
      if( gi1->next_true->suppl.part.conn_id == conn_id ) {
        gi1->suppl.part.stop_true = 1;
      }
      retval = TRUE;
    } else if( gi1->next_true->suppl.part.conn_id == conn_id ) {
      gi1->suppl.part.stop_true = 1;
    } else if( (gi1->next_true != gi2) && ((gi1->suppl.part.type != GI_TYPE_TFN) || (gi1->varname == NULL)) ) {
      retval |= gen_item_connect( gi1->next_true, gi2, conn_id );
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
      char         str[USER_MSG_LENGTH];
      unsigned int rv;
      gen_item_stringify( gi, str, USER_MSG_LENGTH );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Resolving generate item, %s for inst: %s", str, obf_inst( inst->name ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_STMT :
        gitem_link_add( gen_item_create_stmt( gi->elem.stmt ), &(inst->gitem_head), &(inst->gitem_tail) );
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_INST :
        instance_copy( gi->elem.inst, inst, gi->elem.inst->name, gi->elem.inst->range, FALSE );
        gen_item_resolve( gi->next_true, inst );
        break;

      case GI_TYPE_TFN :
        if( gi->varname != NULL ) {
          char         inst_name[4096];
          vsignal*     genvar;
          func_unit*   found_funit;
          unsigned int rv;
          if( !scope_find_signal( gi->varname, inst->funit, &genvar, &found_funit, 0 ) ) {
            rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find variable %s in module %s",
                           obf_sig( gi->varname ), obf_funit( inst->funit->name ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "gen_item Throw B\n" );
            Throw 0;
          }
          rv = snprintf( inst_name, 4096, "%s[%d]", gi->elem.inst->name, vector_to_int( genvar->value ) );
          assert( rv < 4096 );
          (void)instance_parse_add( &inst, inst->funit, gi->elem.inst->funit, inst_name, NULL, FALSE, TRUE );
          rv = snprintf( inst_name, 4096, "%s.%s[%d]", inst->name, gi->elem.inst->name, vector_to_int( genvar->value ) );
          assert( rv < 4096 );
          if( (child = instance_find_scope( inst, inst_name, TRUE )) != NULL ) {
            inst_parm_add_genvar( genvar, child );
          }
        } else {
          char         inst_name[4096];
          unsigned int rv;
          (void)instance_parse_add( &inst, inst->funit, gi->elem.inst->funit, gi->elem.inst->name, NULL, FALSE, TRUE );
          rv = snprintf( inst_name, 4096, "%s.%s", inst->name, gi->elem.inst->name );
          assert( rv < 4096 );
          child = instance_find_scope( inst, inst_name, TRUE );
        }
        gen_item_resolve( gi->next_true, child );
        gen_item_resolve( gi->next_false, inst );
        break;

      case GI_TYPE_BIND :
        varname = gen_item_calc_signal_name( gi->varname, inst->funit, gi->elem.expr->line, FALSE );
        switch( gi->elem.expr->op ) {
          case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    varname, gi->elem.expr, inst->funit );  break;
          case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        varname, gi->elem.expr, inst->funit );  break;
          case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, varname, gi->elem.expr, inst->funit );  break;
          case EXP_OP_DISABLE   :  bind_add( 1,                 varname, gi->elem.expr, inst->funit );  break;
          default               :  bind_add( 0,                 varname, gi->elem.expr, inst->funit );  break;
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

 Recursively resolves all generate items in the design.  This is called at a specific point
 in the binding process.
*/
void generate_resolve(
  funit_inst* root  /*!< Pointer to current instance in instance tree to resolve for */
) { PROFILE(GENERATE_RESOLVE);

  funit_inst* curr_child;  /* Pointer to current child to resolve for */

  if( root != NULL ) {

    /* Resolve ourself */
    if( root->funit != NULL ) {
      gitem_link* curr_gi = root->funit->gitem_head;
      while( curr_gi != NULL ) {
        gen_item_resolve( curr_gi->gi, root );
        curr_gi = curr_gi->next;
      }
    }

    /* Resolve children */
    curr_child = root->child_head;
    while( curr_child != NULL ) {
      generate_resolve( curr_child );
      curr_child = curr_child->next;
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


/*
 $Log$
 Revision 1.74  2008/08/28 04:37:18  phase1geo
 Starting to add support for exclusion output and exclusion IDs to generated
 reports.  These changes should break regressions.  Checkpointing.

 Revision 1.73  2008/08/27 23:06:00  phase1geo
 Starting to make updates for supporting command-line exclusions.  Signals now
 have a unique ID associated with them in the CDD file.  Checkpointing.

 Revision 1.72  2008/08/18 23:07:26  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.68.2.1  2008/07/10 22:43:51  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.70  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.69  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.68  2008/05/30 05:38:31  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.67.2.1  2008/04/30 23:12:31  phase1geo
 Fixing simulation issues.

 Revision 1.67  2008/04/15 20:37:11  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.66  2008/04/06 05:46:54  phase1geo
 Another regression memory deallocation fix.  Updates to regression files.

 Revision 1.65  2008/04/06 05:24:17  phase1geo
 Fixing another regression memory problem.  Updated regression files
 accordingly.  Checkpointing.

 Revision 1.64  2008/03/31 18:39:08  phase1geo
 Fixing more regression issues related to latest code modifications.  Checkpointing.

 Revision 1.63  2008/03/21 21:16:41  phase1geo
 Removing UNUSED_* types in lexer due to a bug that was found in the usage of
 ignore_mode in the lexer (a token that should have been ignored was not due to
 the parser's need to examine the created token for branch traversal purposes).
 This cleans up the parser also.

 Revision 1.62  2008/03/18 21:36:24  phase1geo
 Updates from regression runs.  Regressions still do not completely pass at
 this point.  Checkpointing.

 Revision 1.61  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.60  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.59  2008/03/14 22:00:19  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.58  2008/03/12 21:11:48  phase1geo
 More updates.

 Revision 1.57  2008/03/12 21:10:21  phase1geo
 More exception handling updates.

 Revision 1.56  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

 Revision 1.55  2008/03/04 00:09:20  phase1geo
 More exception handling.  Checkpointing.

 Revision 1.54  2008/02/29 23:58:19  phase1geo
 Continuing to work on adding exception handling code.

 Revision 1.53  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.52  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.51  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.50  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.49  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.48  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.47  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.46  2007/07/26 17:05:15  phase1geo
 Fixing problem with static functions (vector data associated with expressions
 were not being allocated).  Regressions have been run.  Only two failures
 in total still to be fixed.

 Revision 1.45  2007/07/18 22:39:17  phase1geo
 Checkpointing generate work though we are at a fairly broken state at the moment.

 Revision 1.44  2007/07/18 02:15:04  phase1geo
 Attempts to fix a problem with generating instances with hierarchy.  Also fixing
 an issue with named blocks in generate statements.  Still some work to go before
 regressions are passing again, however.

 Revision 1.43  2007/03/15 22:39:05  phase1geo
 Fixing bug in unnamed scope binding.

 Revision 1.42  2006/10/16 21:34:46  phase1geo
 Increased max bit width from 1024 to 65536 to allow for more room for memories.
 Fixed issue with enumerated values being explicitly assigned unknown values and
 created error output message when an implicitly assigned enum followed an explicitly
 assign unknown enum value.  Fixed problem with generate blocks in different
 instantiations of the same module.  Fixed bug in parser related to setting the
 curr_packed global variable correctly.  Added enum2 and enum2.1 diagnostics to test
 suite to verify correct enumerate behavior for the changes made in this checkin.
 Full regression now passes.

 Revision 1.41  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.40  2006/10/13 15:56:02  phase1geo
 Updating rest of source files for compiler warnings.

 Revision 1.39  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.38  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.37  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.36  2006/09/06 22:09:22  phase1geo
 Fixing bug with multiply-and-op operation.  Also fixing bug in gen_item_resolve
 function where an instance was incorrectly being placed into a new instance tree.
 Full regression passes with these changes.  Also removed verbose output.

 Revision 1.35  2006/09/05 21:00:45  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.34  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.33  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.32  2006/08/25 22:49:45  phase1geo
 Adding support for handling generated hierarchical names in signals that are outside
 of generate blocks.  Added support for op-and-assigns in generate for loops as well
 as normal for loops.  Added generate11.4 and for3 diagnostics to regression suite
 to verify this new behavior.  Full regressions have not been verified with these
 changes however.  Checkpointing.

 Revision 1.31  2006/08/25 18:25:24  phase1geo
 Modified gen39 and gen40 to not use the Verilog-2001 port syntax.  Fixed problem
 with detecting implicit .name and .* syntax.  Fixed op-and-assign report output.
 Added support for 'typedef', 'struct', 'union' and 'enum' syntax for SystemVerilog.
 Updated user documentation.  Full regression completely passes now.

 Revision 1.30  2006/08/24 22:25:12  phase1geo
 Fixing issue with generate expressions within signal hierarchies.  Also added
 ability to parse implicit named and * port lists.  Added diagnostics to regressions
 to verify this new ability.  Full regression passes.

 Revision 1.29  2006/08/24 03:39:02  phase1geo
 Fixing some issues with new static_lexer/parser.  Working on debugging issue
 related to the generate variable mysteriously losing its vector data.

 Revision 1.28  2006/08/23 22:18:20  phase1geo
 Adding static expression parser/lexer for parsing generate signals.  Integrated
 this into gen_item.  First attempt and hasn't been tested, so I'm sure its full
 of bugs at the moment.

 Revision 1.27  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.26  2006/08/14 04:19:56  phase1geo
 Fixing problem with generate11* diagnostics (generate variable used in
 signal name).  These tests pass now but full regression hasn't been verified
 at this point.

 Revision 1.25  2006/08/02 22:28:32  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.24  2006/08/01 18:05:13  phase1geo
 Adding more diagnostics to test generate item structure connectivity.  Fixing
 bug in funit_find_signal function to search the function (instead of the instance
 for for finding a signal to bind).

 Revision 1.23  2006/07/30 04:30:50  phase1geo
 Adding generate8.2 diagnostic which uses nested generate loops.  The problem
 with Covered with this diagnostic is not in the nested for loops but rather it
 currently does not bind generate variables with TFN generate items.

 Revision 1.22  2006/07/29 20:53:43  phase1geo
 Fixing some code related to generate statements; however, generate8.1 is still
 not completely working at this point.  Full regression passes for IV.

 Revision 1.21  2006/07/28 22:42:51  phase1geo
 Updates to support expression/signal binding for expressions within a generate
 block statement block.

 Revision 1.20  2006/07/27 21:19:27  phase1geo
 Small updates.

 Revision 1.19  2006/07/27 18:02:22  phase1geo
 More diagnostic additions and upgraded the generate item display functionality
 for better debugging using this feature.  We are about to forge into some new
 territory with regard to generate blocks, so we will tag at this point.

 Revision 1.18  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.17  2006/07/27 02:04:30  phase1geo
 Fixing problem with parameter usage in a generate block for signal sizing.

 Revision 1.16  2006/07/26 06:22:27  phase1geo
 Fixing rest of issues with generate6 diagnostic.  Still need to know if I
 have broken regressions or not and there are plenty of cases in this area
 to test before I call things good.

 Revision 1.15  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.14  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.13  2006/07/24 13:35:36  phase1geo
 More generate updates.

 Revision 1.12  2006/07/22 03:57:07  phase1geo
 Adding support for parameters within generate blocks.  Adding more diagnostics
 to verify statement support and parameter usage (signal sizing).

 Revision 1.11  2006/07/21 22:39:01  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.10  2006/07/21 20:12:46  phase1geo
 Fixing code to get generated instances and generated array of instances to
 work.  Added diagnostics to verify correct functionality.  Full regression
 passes.

 Revision 1.9  2006/07/21 17:47:09  phase1geo
 Simple if and if-else generate statements are now working.  Added diagnostics
 to regression suite to verify these.  More testing to follow.

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

