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
 \file     symtable.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/3/2002

 \par VCD Files
 A VCD (Value Change Dump) file is broken into two parts:  a definition section and a
 value change section.  The definition section may contain various information about
 the tool that generated the dumpfile (ignored by Covered), the date that the dumpfile
 was created (ignored by Covered), comments about the dumpfile (ignored by Covered),
 and the scope and variable definition information (only part of definition section that
 is used by Covered).

 \par
 In the scope and variable definition section, we learn about what variables (these
 correspond to Covered's signals) are dumped and their corresponding VCD symbol.  A
 VCD symbol can be any sequence of printable ASCII characters such that a sequence
 is unique to the variable that it represents.  In some cases, more than one variable
 is represented by the same VCD symbol.  This occurs when two differently named variables
 actually reference the same value (as in a Verilog port -- the name changes when moving
 from one module to another but it contains the same value).  Because all references to
 variables in the simulation section of the VCD file use the VCD symbol as a lookup
 mechanism, we need to store this variable information (symbol name, its value width,
 pointer to the variable(s) being referrenced for that symbol name, etc.) in some sort
 of quick access lookup table.  This table is referred to as a symtable in Covered.

 \par
 In the simulation section of a VCD file, a number of problems can arise when parsing
 symbols within a timestep.  First, when a symbol is encountered, it may pertain to
 information for several symbols.  Therefore, we need to take the value change information
 and apply it to all variables (Covered signals) that correspond to that symbol.
 Secondly, the value change information for one VCD symbol may be output several times
 to the dumpfile.  This behavior is unnecessary but some simulators do this whenever a
 variable changes value while others only output the last value of a VCD symbol prior to
 changing timesteps.  Because of this, Covered will override the value change information
 of a VCD symbol if multiple lines are found, causing only the last value for a particular
 VCD symbol to be used.

 \par The Symtable Structure
 A symtable is a tree-like structure that is used to hold three pieces of information
 that are used during the simulation phase of the score command:

 \par
 -# The name of the VCD symbol that a symtable entry represents
 -# A list of pointers to signals which are represented by a VCD symbol.
 -# A temporary storage facility to hold value change information for a particular
    VCD symbol.

 \par
 The tree structure itself consists of nodes, one node per VCD symbol (with the exception
 of the root node -- this will be explained later) and each node contains an array of 94
 pointers to other nodes.  Having an array of 94 pointers allows us to use the name of the
 VCD symbol as the lookup index into the table.  Because VCD symbols are allowed to use any
 combination of printable ASCII characters, the length of a VCD symbol (even for a large
 design) is usually between 1-4 characters.  This means that finding the information for any
 given signal only takes between 0 and 3 node hops, making VCD symbol access during the
 simulation phase extremely fast.

 \par
 The symtable is initially formed by creating a root node, the root node does not contain
 any symbol information (because the only symbol it could hold is the NULL character which
 is not an ASCII printable character).  Once the root node has been created, parsing of the
 VCD definition section begins.  When the first VCD symbol is encountered (let's say the
 character is '!'), we perform a symbol lookup by accessing the 33rd element of the root
 array (33 is the decimal form of the '!' symbol).  In this case the element is a pointer
 to NULL; therefore, a new node is created.  We then grab the next character in the VCD
 symbol name which is NULL.  Because we have hit NULL, we know that the current node that
 we are in (the newly created node) is the node for our VCD symbol.  Therefore, we initialize
 the new node with the VCD symbol information for our symbol (note that we do not need to
 store the VCD symbol string in the node because it is used as an index).  This process continues
 until we have processed all VCD symbols in the VCD file, creating a tree structure that remains
 in memory until the scoring process is complete.

 \par
 When the simulation section is being parsed, VCD symbols are looked up in the same way that they
 were stored.  When we hit the NULL character in the VCD name string, we have found the node
 that contains the information for that symbol.  We then store the new value into the node.

 \par The Timestep Array
 When a timestep is found in the VCD file, we need to perform a simulation of all signal changes
 made during that timestep.  If the symtable structure was the only structure used to find all
 signals that changed during that timestep, we would need to perform a complete traversal of the
 tree for each timestep (i.e., we would need to check every signal in the design to see if it had
 changed).  This is unnecessary and results in bad performance.

 \par
 To make this lookup of changed signals more efficient, an array called "timestep_tab" is used.  This
 array is an array of pointers to symtable tree nodes, one entry for each node in the symtable tree.
 The array is allocated after the symtable tree has been fully populated and is destroyed at the very
 end of the score command.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>

#include "defines.h"
#include "fsm.h"
#include "link.h"
#include "sim.h"
#include "symtable.h"
#include "util.h"
#include "vsignal.h"


extern const exp_info exp_op_info[EXP_OP_NUM];


/*!
 Pointer to the VCD symbol table.  Please see the file description for how this
 structure is used.
*/
symtable* vcd_symtab = NULL;

/*!
 Maintains current number of nodes in the VCD symbol table.  This value is used
 to create the appropriately sized timestep_tab array.
*/
int vcd_symtab_size = 0;

/*!
 Pointer to the current timestep table array.  Please see the file description for
 how this structure is used.
*/
symtable** timestep_tab = NULL;

/*!
 Maintains the current number of elements in the timestep_tab array that need to be
 evaluated after simulation for a timestep.
*/
static int postsim_size = 0;


/*!
 Creates and adds the specified symtable signal structure to the sym_sig
 list for the specified symtab.
*/
static void symtable_add_sym_sig(
  symtable* symtab,  /*!< Pointer to symbol table entry to initialize */
  vsignal*  sig,     /*!< Pointer to signal that will be stored in the symtable list */
  int       msb,     /*!< Most-significant bit of symbol entry */
  int       lsb      /*!< Least-significant bit of symbol entry */
) { PROFILE(SYMTABLE_ADD_SYM_SIG);

  sym_sig* new_ss;  /* Pointer to newly created sym_sig structure */

  /* Create new sym_sig structure */
  new_ss       = (sym_sig*)malloc_safe( sizeof( sym_sig ) );
  new_ss->sig  = sig;
  new_ss->msb  = msb;
  new_ss->lsb  = lsb;
  new_ss->next = NULL;

  /* Add new structure to head of symtable list */
  if( symtab->entry.sig != NULL ) {
    new_ss->next = symtab->entry.sig;
  }

  assert( (symtab->entry_type == 0) || (symtab->entry_type == 1) );

  /* Populate symtab entry */
  symtab->entry.sig  = new_ss;
  symtab->entry_type = 1;

  PROFILE_END;

}

/*!
 Creates and adds the specified symtable expression structure to the sym_exp
 list for the specified symtab.
*/
static void symtable_add_sym_exp(
  symtable*   symtab,  /*!< Pointer to symbol table entry to initialize */
  expression* exp,     /*!< Pointer to expression that will be stored in symtable list */
  char        action   /*!< Action to perform when expression is assigned */
) { PROFILE(SYMTABLE_ADD_SYM_EXP);

  sym_exp* new_se;  /* Pointer to newly created sym_exp structure */

  /* Create new sym_exp structure */
  new_se         = (sym_exp*)malloc_safe( sizeof( sym_exp ) );
  new_se->exp    = exp;
  new_se->action = action;
  new_se->next   = NULL;

  /* Add new structure to head of symtable list */
  if( symtab->entry.exp != NULL ) {
    new_se->next = symtab->entry.exp;
  }

  assert( (symtab->entry_type == 0) || (symtab->entry_type == 2) );

  /* Populate symtab entry */
  symtab->entry.exp  = new_se;
  symtab->entry_type = 2;

  PROFILE_END;

}

/*!
 Adds the specified symtable FSM table structure to the specified symtable entry.
*/
static void symtable_add_sym_fsm(
  symtable*   symtab,  /*!< Pointer to symbol table entry to initialize */
  fsm*        table    /*!< Pointer to FSM table that will be stored in symtable list */
) { PROFILE(SYMTABLE_ADD_SYM_FSM);

  assert( (symtab->entry_type == 0) || (symtab->entry_type == 3) );

  /* Populate symtab entry */
  symtab->entry.table = table;
  symtab->entry_type  = 3;

  PROFILE_END;

}

/*!
 Initializes the contents of a symbol table entry.
*/
static void symtable_init(
  symtable* symtab,  /*!< Pointer to symbol table entry to initialize */
  int       msb,     /*!< Most-significant bit of symbol entry */
  int       lsb      /*!< Least-significant bit of symbol entry */
) { PROFILE(SYMTABLE_INIT);

  /* Allocate and initialize the entry */
  symtab->size     = (msb - lsb) + 2;
  symtab->value    = (char*)malloc_safe( symtab->size );
  symtab->value[0] = '\0';

  PROFILE_END;

}

/*!
 \return Returns a pointer to the newly created symbol table entry.

 Creates a new symbol table entry and returns a pointer to the
 newly created structure.
*/
symtable* symtable_create() { PROFILE(SYMTABLE_CREATE);

  symtable* symtab;  /* Pointer to new symtable entry */
  int       i;       /* Loop iterator */

  symtab             = (symtable*)malloc_safe( sizeof( symtable ) );
  symtab->entry.sig  = NULL;
  symtab->entry_type = 0;
  symtab->value      = NULL;
  for( i=0; i<94; i++ ) {
    symtab->table[i] = NULL;
  }

  PROFILE_END;

  return( symtab );

}

/*!
 \return Returns a pointer to the symtable to use for the new entry.
*/
static symtable* symtable_get_table(
  const char* sym  /*!< Symbol to use as a lookup mechanism into the table */
) { PROFILE(SYMTABLE_GET_TABLE);

  symtable*   curr;  /* Pointer to current symtable entry */
  const char* ptr;   /* Pointer to current character in sym */

  assert( vcd_symtab != NULL );
  assert( sym[0]     != '\0' );

  curr = vcd_symtab;
  ptr  = sym;

  while( *ptr != '\0' ) {
    if( curr->table[(int)(*ptr) - 33] == NULL ) {
      curr->table[(int)(*ptr) - 33] = symtable_create();
    }
    curr = curr->table[(int)(*ptr) - 33];
    ptr++;
  }

  PROFILE_END;

  return( curr );

}

/*!
 Using the symbol as a unique ID, creates a new symtable element for specified information
 and places it into the binary tree.
*/
void symtable_add_signal(
  const char* sym,  /*!< VCD symbol for the specified signal */
  vsignal*    sig,  /*!< Pointer to signal corresponding to the specified symbol */
  int         msb,  /*!< Most significant bit of variable to set */
  int         lsb   /*!< Least significant bit of variable to set */
) { PROFILE(SYMTABLE_ADD_SIGNAL);

  symtable* curr;  /* Pointer to current symtable entry */

  /* Get a table entry */
  curr = symtable_get_table( sym );

  if( curr->entry.sig == NULL ) {
    if( msb < lsb ) {
      symtable_init( curr, lsb, msb );
    } else {
      symtable_init( curr, msb, lsb );
    }
  }

  symtable_add_sym_sig( curr, sig, msb, lsb );

  /* 
   Finally increment the number of entries in the root table structure.
  */
  vcd_symtab_size++;

  PROFILE_END;

}

/*!
 Using the symbol as a unique ID, creates a new symtable element for specified information
 and places it into the binary tree.
*/
void symtable_add_expression(
  const char* sym,    /*!< VCD symbol for the specified signal */
  expression* exp,    /*!< Pointer to signal corresponding to the specified symbol */
  char        action  /*!< Specifies the type of action to perform on the given expression */
) { PROFILE(SYMTABLE_ADD_EXPRESSION);

  symtable* curr;  /* Pointer to current symtable entry */
  int       msb;   /* Most significan bit of vector that will be stored */

  /* Get a table entry */
  curr = symtable_get_table( sym );

  /* Calculate the MSB */
  if( (action == 'l') || exp_op_info[exp->op].suppl.is_event || exp_op_info[exp->op].suppl.is_unary ) {
    msb = 0;
  } else if( exp_op_info[exp->op].suppl.is_comb == OTHER_COMB ) {
    msb = 3;
  } else {
    msb = 2;
  }

  if( curr->entry.exp == NULL ) {
    symtable_init( curr, msb, 0 );
  }

  symtable_add_sym_exp( curr, exp, action );

  /* 
   Finally increment the number of entries in the root table structure.
  */
  vcd_symtab_size++;

  PROFILE_END;

}

/*!
 Using the symbol as a unique ID, creates a new symtable element for specified information and
 places it into the binary tree.
*/
void symtable_add_memory(
  const char* sym,     /*!< VCD symbol for the specified signal */
  expression* exp,     /*!< Pointer to signal corresponding to the specified symbol */
  char        action,  /*!< Specifies the type of action to perform on the given expression */
  int         msb      /*!< MSB of signal from dumpfile */
) { PROFILE(SYMTABLE_ADD_MEMORY);

  symtable* curr;  /* Pointer to current symtable entry */

  /* Get a table entry */
  curr = symtable_get_table( sym );

  if( curr->entry.exp == NULL ) {
    symtable_init( curr, msb, 0 );
  }

  symtable_add_sym_exp( curr, exp, action );

  /*
   Finally increment the number of entries in the root table structure.
  */
  vcd_symtab_size++;

  PROFILE_END;

}

/*!
 Using the symobl as a unique ID, creates a new symtable element for specified information
 and places it into the lookup tree.
*/
void symtable_add_fsm(
  const char* sym,    /*!< VCD symbol for the specified signal */
  fsm*        table,  /*!< Pointer to FSM table to set */
  int         msb,    /*!< Most-significant bit position to set */
  int         lsb     /*!< Least-significant bit position to set */
) { PROFILE(SYMTABLE_ADD_FSM);

  symtable* curr;  /* Pointer to current symtable entry */

  /* Get a table entry */
  curr = symtable_get_table( sym );

  if( curr->entry.table == NULL ) {
    symtable_init( curr, msb, lsb );
  }

  symtable_add_sym_fsm( curr, table );

  /*
   Finally increment the number of entries in the root table structure.
  */
  vcd_symtab_size++;

  PROFILE_END;

}

/*!
 Performs a binary search of the specified tree to find all matching symtable entries.
 When the signal is found, the specified value is assigned to the symtable entry.
*/
void symtable_set_value(
  const char* sym,   /*!< Name of symbol to find in the table */
  const char* value  /*!< Value to set symtable entry to when match found */
) { PROFILE(SYMTABLE_SET_VALUE);

  symtable*   curr;         /* Pointer to current symtable */
  const char* ptr;          /* Pointer to current character in symbol */
  bool        set = FALSE;  /* Specifies if this symtable entry has been set this timestep yet */

  assert( vcd_symtab != NULL );
  assert( sym[0] != '\0' );

  curr = vcd_symtab;
  ptr  = sym;

  while( (curr != NULL) && (*ptr != '\0') ) {
    curr = curr->table[(int)(*ptr) - 33];
    ptr++;
  }

  if( (curr != NULL) && (curr->value != NULL) ) {

    if( curr->value[0] != '\0' ) {
      set = TRUE;
    }

    /* printf( "strlen( value ): %d, curr->size: %d\n", strlen( value ), curr->size ); */
    assert( strlen( value ) < curr->size );     /* Useful for debugging but not necessary */
    strcpy( curr->value, value );

    if( !set ) {

      /* Place in postsim queue */
      timestep_tab[postsim_size] = curr;
      postsim_size++;
   
    }

  }

  PROFILE_END;

}

/*!
 \throws anonymous vsignal_vcd_assign

 Traverses simulation symentry array, assigning stored string value to the
 stored signal.
*/
void symtable_assign(
  const sim_time* time  /*!< Pointer to current simulation time structure */
) { PROFILE(SYMTABLE_ASSIGN);

  symtable* curr;  /* Pointer to current symtable entry */
  int       i;     /* Loop iterator */

  for( i=0; i<postsim_size; i++ ) {
    curr = timestep_tab[i];
    if( curr->entry_type == 1 ) {
      sym_sig* sig = curr->entry.sig;
      while( sig != NULL ) {
        vsignal_vcd_assign( sig->sig, curr->value, sig->msb, sig->lsb, time );
        sig = sig->next;
      }
    } else if( curr->entry_type == 2 ) {
      sym_exp* exp = curr->entry.exp;
      while( exp != NULL ) {
        expression_vcd_assign( exp->exp, exp->action, curr->value );
        exp = exp->next;
      }
    } else if( curr->entry_type == 3 ) {
      fsm_vcd_assign( curr->entry.table, curr->value );
    }
    curr->value[0] = '\0';
  }
  postsim_size = 0;

  PROFILE_END;

}

/*!
 Recursively deallocates all elements of specifies symbol table.
*/ 
void symtable_dealloc(
  symtable* symtab  /*!< Pointer to root of symtable to clear */
) { PROFILE(SYMTABLE_DEALLOC);

  if( symtab != NULL ) {

    int i;

    for( i=0; i<94; i++ ) {
      symtable_dealloc( symtab->table[i] );
    }

    if( symtab->value != NULL ) {
      free_safe( symtab->value, symtab->size );
    }

    if( symtab->entry_type == 1 ) {

      sym_sig* curr;
      sym_sig* tmp;

      /* Remove sym_sig list */
      curr = symtab->entry.sig;
      while( curr != NULL ) {
        tmp = curr->next;
        free_safe( curr, sizeof( sym_sig ) );
        curr = tmp;
      }

    } else if( symtab->entry_type == 2 ) {

      sym_exp* curr;
      sym_exp* tmp;

      /* Remove sym_exp list */
      curr = symtab->entry.exp;
      while( curr != NULL ) {
        tmp = curr->next;
        free_safe( curr, sizeof( sym_exp ) );
        curr = tmp;
      }

    }

    free_safe( symtab, sizeof( symtable ) );

  }

  PROFILE_END;

}

