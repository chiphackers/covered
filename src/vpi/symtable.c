/*!
 \file     symtable.c
 \author   Trevor Williams  (trevorw@charter.net)
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
 of the root node -- this will be explained later) and each node contains an array of 256
 pointers to other nodes.  Having an array of 256 pointers allows us to use the name of the
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

 \par
 Two indices are used to maintain the array, presim_size and postsim_size.  All signals that changed
 during the current timestep and need to be assigned to their appropriate signals before simulation
 occurs for that timestep are placed in the lower elements of this array (i.e., starting at zero and
 moving upperward).  The presim_size indicates how many elements are in the lower elements of the
 array.  All signals that changed during the current timestep and need to be assigned to their
 appropriate signals after simulation occurs for that timestep are placed in the upper elements of
 this array (i.e., starting at the largest index and moving downward).  The postsim_size indicates how
 many elements are in the upper elements of the array.  Both the presim_size and postsim_size signals
 are set to zero before the next timestep.
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
#include "symtable.h"
#include "util.h"
#include "vsignal.h"
#include "link.h"
#include "sim.h"

/*!
 Pointer to the VCD symbol table.  Please see the file description for how this
 structure is used.
*/
symtable*  vcd_symtab      = NULL;

/*!
 Maintains current number of nodes in the VCD symbol table.  This value is used
 to create the appropriately sized timestep_tab array.
*/
int        vcd_symtab_size = 0;

/*!
 Pointer to the current timestep table array.  Please see the file description for
 how this structure is used.
*/
symtable** timestep_tab    = NULL;

/*!
 Maintains the current number of elements in the timestep_tab array that need to be
 evaluated prior to simulation for a timestep.
*/
int        presim_size     = 0;

/*!
 Maintains the current number of elements in the timestep_tab array that need to be
 evaluated after simulation for a timestep.
*/
int        postsim_size    = 0;


/*!
 \param symtab  Pointer to symbol table entry to initialize.
 \param sig     Pointer to signal that will be stored in the symtable list.
 \param msb     Most-significant bit of symbol entry.
 \param lsb     Least-significant bit of symbol entry.

 Creates and adds the specified symtable signal structure to the sym_sig
 list for the specified symtab.
*/
void symtable_add_sym_sig( symtable* symtab, vsignal* sig, int msb, int lsb ) {

  sym_sig* new_ss;  /* Pointer to newly created sym_sig structure */

  /* Create new sym_sig structure */
  new_ss       = (sym_sig*)malloc_safe( sizeof( sym_sig ), __FILE__, __LINE__ );
  new_ss->sig  = sig;
  new_ss->msb  = msb;
  new_ss->lsb  = lsb;
  new_ss->next = NULL;

  /* Add new structure to symtable list */
  if( symtab->sig_head == NULL ) {
    symtab->sig_head = symtab->sig_tail = new_ss;
  } else {
    symtab->sig_tail->next = new_ss;
    symtab->sig_tail       = new_ss;
  }

}

/*!
 \param symtab  Pointer to symbol table entry to initialize.
 \param msb     Most-significant bit of symbol entry.
 \param lsb     Least-significant bit of symbol entry.

 Initializes the contents of a symbol table entry.
*/
void symtable_init( symtable* symtab, int msb, int lsb ) {

  symtab->value    = (char*)malloc_safe( ((msb - lsb) + 2), __FILE__, __LINE__ );
  symtab->value[0] = '\0';
  symtab->size     = (msb - lsb) + 2;

}

/*!
 \return Returns a pointer to the newly created symbol table entry.

 Creates a new symbol table entry and returns a pointer to the
 newly created structure.
*/
symtable* symtable_create() {

  symtable* symtab;  /* Pointer to new symtable entry */
  int       i;       /* Loop iterator                 */

  symtab           = (symtable*)malloc_safe( sizeof( symtable ), __FILE__, __LINE__ ); 
  symtab->sig_head = NULL;
  symtab->sig_tail = NULL;
  symtab->value    = NULL;
  for( i=0; i<256; i++ ) {
    symtab->table[i] = NULL;
  }

  return( symtab );

}

/*!
 \param sym     VCD symbol for the specified signal.
 \param sig     Pointer to signal corresponding to the specified symbol.
 \param msb     Most significant bit of variable to set.
 \param lsb     Least significant bit of variable to set.

 Using the symbol as a unique ID, creates a new symtable element for specified information
 and places it into the binary tree.
*/
void symtable_add( char* sym, vsignal* sig, int msb, int lsb ) {

  symtable* curr;  /* Pointer to current symtable entry   */
  char*     ptr;   /* Pointer to current character in sym */

  assert( vcd_symtab != NULL );
  assert( sym[0]     != '\0' );
  assert( sig->value != NULL );

  curr = vcd_symtab;
  ptr  = sym;

  while( *ptr != '\0' ) {
    if( curr->table[(int)*ptr] == NULL ) {
      curr->table[(int)*ptr] = symtable_create();
    }
    curr = curr->table[(int)*ptr];
    ptr++;
  }

  if( curr->sig_head == NULL ) {
    symtable_init( curr, msb, lsb );
  }

  symtable_add_sym_sig( curr, sig, msb, lsb );

  /* 
   Finally increment the number of entries in the root table structure.
  */
  vcd_symtab_size++;

}

/*!
 \param sym     Name of symbol to find in the table.
 \param value   Value to set symtable entry to when match found.

 Performs a binary search of the specified tree to find all matching symtable entries.
 When the signal is found, the specified value is assigned to the symtable entry.
*/
void symtable_set_value( char* sym, char* value ) {

  symtable* curr;         /* Pointer to current symtable                                     */
  sym_sig*  sig;          /* Pointer to current sym_sig in list                              */
  char*     ptr;          /* Pointer to current character in symbol                          */
  bool      set = FALSE;  /* Specifies if this symtable entry has been set this timestep yet */

  assert( vcd_symtab != NULL );
  assert( sym[0] != '\0' );

  curr = vcd_symtab;
  ptr  = sym;

  while( (curr != NULL) && (*ptr != '\0') ) {
    curr = curr->table[(int)*ptr];
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

      /*
       See if current signal is to be placed in presim queue or postsim queue and
       put it there.
      */
      sig = curr->sig_head;
      while( (sig != NULL) && (vsignal_get_wait_bit( sig->sig ) == 0) ) {
        sig = sig->next;
      }

      /* None of the signals are wait signals, place in postsim queue */
      if( sig == NULL ) {
        timestep_tab[((vcd_symtab_size - 1) - postsim_size)] = curr;
        postsim_size++;
      } else {
        timestep_tab[presim_size] = curr;
        presim_size++;
      }
   
    }

  }

}

/*!
 \param presim  If set to TRUE, assigns all signals for pre-simulation (else assign all signals
                for post-simulation.

 Traverses simulation symentry array, assigning stored string value to the
 stored signal.
*/
void symtable_assign( bool presim ) {

  symtable* curr;  /* Pointer to current symtable entry  */
  sym_sig*  sig;   /* Pointer to current sym_sig in list */
  int       i;     /* Loop iterator                      */

  if( presim ) {
    for( i=0; i<presim_size; i++ ) {
      curr = timestep_tab[i];
      sig = curr->sig_head;
      while( sig != NULL ) {
        vsignal_vcd_assign( sig->sig, curr->value, sig->msb, sig->lsb );
        sig = sig->next;
      }
      curr->value[0] = '\0';
    }
    presim_size = 0;
  } else {
    for( i=(vcd_symtab_size - 1); i>=(vcd_symtab_size - postsim_size); i-- ) {
      curr = timestep_tab[i];
      sig = curr->sig_head;
      while( sig != NULL ) {
        vsignal_vcd_assign( sig->sig, curr->value, sig->msb, sig->lsb );
        sig = sig->next;
      }
      curr->value[0] = '\0';
    }
    postsim_size = 0;
  }

}

/*!
 \param symtab  Pointer to root of symtable to clear.

 Recursively deallocates all elements of specifies symbol table.
*/ 
void symtable_dealloc( symtable* symtab ) {

  sym_sig* curr;  /* Pointer to current sym_sig in list */
  sym_sig* tmp;   /* Temporary pointer to sym_sig       */
  int      i;     /* Loop iterator                      */

  if( symtab != NULL ) {

    for( i=0; i<256; i++ ) {
      symtable_dealloc( symtab->table[i] );
    }

    if( symtab->value != NULL ) {
      free_safe( symtab->value );
    }

    /* Remove sym_sig list */
    curr = symtab->sig_head;
    while( curr != NULL ) {
      tmp = curr->next;
      free_safe( curr );
      curr = tmp;
    }

    free_safe( symtab );

  }

}

/*
 $Log$
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

