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

  /* Populate symtab entry */
  symtab->entry.sig  = new_ss;
  symtab->entry_type = 0;

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

  /* Populate symtab entry */
  symtab->entry.exp  = new_se;
  symtab->entry_type = 1;

  PROFILE_END;

}

/*!
 Adds the specified symtable FSM table structure to the specified symtable entry.
*/
static void symtable_add_sym_fsm(
  symtable*   symtab,  /*!< Pointer to symbol table entry to initialize */
  fsm*        table    /*!< Pointer to FSM table that will be stored in symtable list */
) { PROFILE(SYMTABLE_ADD_SYM_FSM);

  /* Populate symtab entry */
  symtab->entry.table = table;
  symtab->entry_type  = 2;

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

  symtab            = (symtable*)malloc_safe( sizeof( symtable ) );
  symtab->entry.sig = NULL;
  symtab->value     = NULL;
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
    symtable_init( curr, msb, lsb );
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
  sym_sig*  sig;   /* Pointer to current sym_sig in list */
  int       i;     /* Loop iterator */

  for( i=0; i<postsim_size; i++ ) {
    curr = timestep_tab[i];
    if( curr->entry_type == 0 ) {
      sig = curr->entry.sig;
      while( sig != NULL ) {
        vsignal_vcd_assign( sig->sig, curr->value, sig->msb, sig->lsb, time );
        sig = sig->next;
      }
    } else if( curr->entry_type == 1 ) {
      expression_vcd_assign( curr->entry.exp->exp, curr->entry.exp->action, curr->value );
    } else {
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

    if( symtab->entry_type == 0 ) {

      sym_sig* curr;
      sym_sig* tmp;

      /* Remove sym_sig list */
      curr = symtab->entry.sig;
      while( curr != NULL ) {
        tmp = curr->next;
        free_safe( curr, sizeof( sym_sig ) );
        curr = tmp;
      }

    } else if( symtab->entry_type == 1 ) {

      free_safe( symtab->entry.exp, sizeof( sym_exp ) );

    }

    free_safe( symtab, sizeof( symtable ) );

  }

  PROFILE_END;

}

/*
 $Log$
 Revision 1.43  2008/12/24 21:19:02  phase1geo
 Initial work at getting FSM coverage put in (this looks to be working correctly
 to this point).  Updated regressions per fixes.  Checkpointing.

 Revision 1.42  2008/12/10 00:19:23  phase1geo
 Fixing issues with aedge1 diagnostic (still need to handle events but this will
 be worked on a later time).  Working on sizing temporary subexpression LHS signals.
 This is not complete and does not compile at this time.  Checkpointing.

 Revision 1.41  2008/12/06 06:35:20  phase1geo
 Adding first crack at handling coverage-related information from dumpfile.
 This code is untested.

 Revision 1.40  2008/12/05 23:05:38  phase1geo
 Working on VCD reading side of the inlined coverage handler.  Things don't
 compile at this point and are in limbo.  Checkpointing.

 Revision 1.39  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.36.4.1  2008/07/10 22:43:55  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.37  2008/06/28 03:46:29  phase1geo
 More code updates for warning removal.

 Revision 1.36  2008/03/17 22:02:32  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.35  2008/03/17 05:26:17  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.34  2008/03/13 10:28:55  phase1geo
 The last of the exception handling modifications.

 Revision 1.33  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.32  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.31  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.30  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.29  2007/12/11 23:19:14  phase1geo
 Fixed compile issues and completed first pass injection of profiling calls.
 Working on ordering the calls from most to least.

 Revision 1.28  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.27  2006/11/27 04:11:42  phase1geo
 Adding more changes to properly support thread time.  This is a work in progress
 and regression is currently broken for the moment.  Checkpointing.

 Revision 1.26  2006/10/13 15:56:02  phase1geo
 Updating rest of source files for compiler warnings.

 Revision 1.25  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.24  2006/05/25 12:11:02  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.23  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.22.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.22  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.21  2006/01/26 06:06:37  phase1geo
 Starting to work on formatting the lxt2_read file and improving error output
 to conform to Covered's error-reporting mechanism.

 Revision 1.20  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

 Revision 1.19  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.18  2004/03/30 15:42:15  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.17  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.16  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.15  2003/08/21 21:57:30  phase1geo
 Fixing bug with certain flavors of VCD files that alias signals that have differing
 MSBs and LSBs.  This takes care of the rest of the bugs for the 0.2 stable release.

 Revision 1.14  2003/08/20 22:08:39  phase1geo
 Fixing problem with not closing VCD file after VCD parsing is completed.
 Also fixed memory problem with symtable.c to cause timestep_tab entries
 to only be loaded if they have not already been loaded during this timestep.
 Also added info.h to include list of db.c.

 Revision 1.13  2003/08/18 23:52:54  phase1geo
 Fixing bug in initialization function for a symtable to initialize all 256
 elements of the table array (instead of 255).

 Revision 1.12  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.11  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.10  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.9  2003/01/03 05:52:00  phase1geo
 Adding code to help safeguard from segmentation faults due to array overflow
 in VCD parser and symtable.  Reorganized code for symtable symbol lookup and
 value assignment.

 Revision 1.8  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.7  2002/10/31 23:14:29  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.6  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.5  2002/07/10 03:01:50  phase1geo
 Added define1.v and define2.v diagnostics to regression suite.  Both diagnostics
 now pass.  Fixed cases where constants were not causing proper TRUE/FALSE values
 to be calculated.

 Revision 1.4  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.3  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

