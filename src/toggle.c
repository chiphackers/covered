/*!
 \file     toggle.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
*/

#include <stdio.h>
#include <assert.h>

#include "toggle.h"
#include "defines.h"


extern mod_inst* instance_root;
extern mod_link* mod_head;

/*!
 \param expl   Pointer to expression list to search.
 \param sigl   Pointer to signal list to search.
 \param total  Total number of bits in the design/module.
 \param hit01  Number of bits toggling from 0 to 1 during simulation.
 \param hit10  Number of bits toggling from 1 to 0 during simulation.

 Searches specified expression list and signal list, gathering information 
 about toggled bits.  For each bit that is found in the design, the total
 value is incremented by one.  For each bit that toggled from a 0 to a 1,
 the value of hit01 is incremented by one.  For each bit that toggled from
 a 1 to a 0, the value of hit10 is incremented by one.
*/
void toggle_get_stats( exp_link* expl, sig_link* sigl, float* total, int* hit01, int* hit10 ) {

  exp_link* curr_exp = expl;    /* Current expression being evaluated */
  sig_link* curr_sig = sigl;    /* Current signal being evaluated     */
  
  /* Search signal list */
  while( curr_sig != NULL ) {
    *total = *total + curr_sig->sig->value->width;
    vector_toggle_count( curr_sig->sig->value, hit01, hit10 );
    curr_sig = curr_sig->next;
  }

#ifdef USE_TOGGLE_EXPR
  /* Search expression list */
  while( curr_exp != NULL ) {
    if( (curr_exp->exp->op != EXP_OP_SIG) &&
        (curr_exp->exp->op != EXP_OP_SBIT_SEL) &&
        (curr_exp->exp->op != EXP_OP_MBIT_SEL) ) {
      *total = *total + curr_exp->exp->value->width;
      vector_toggle_count( curr_exp->exp->value, hit01, hit10 );
    }
    curr_exp = curr_exp->next;
  }
#endif
    
}

/*!
 \param ofile        File to output coverage information to.
 \param root         Instance node in the module instance tree being evaluated.
 \param parent_inst  Name of parent instance.

 Displays the toggle instance summarization to the specified file.  Recursively
 iterates through module instance tree, outputting the toggle information that
 is found at that instance.
*/
void toggle_instance_summary( FILE* ofile, mod_inst* root, char* parent_inst ) {

  mod_inst* curr;        /* Pointer to current child module instance of this node */
  float     percent01;   /* Percentage of bits toggling from 0 -> 1               */
  float     percent10;   /* Percentage of bits toggling from 1 -> 0               */
  float     miss01;      /* Number of bits that did not toggle from 0 -> 1        */
  float     miss10;      /* Number of bits that did not toggle from 1 -> 0        */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Calculate for toggle01 information */
  percent01 = ((root->stat->tog01_hit / root->stat->tog_total) * 100);
  miss01    = (root->stat->tog_total - root->stat->tog01_hit);

  /* Calculate for toggle10 information */
  percent10 = ((root->stat->tog10_hit / root->stat->tog_total) * 100);
  miss10    = (root->stat->tog_total - root->stat->tog10_hit);

  fprintf( ofile, "  %-20.20s    %-20.20s    %3d/%3.0f/%3.0f      %3.0f%%            %3d/%3.0f/%3.0f      %3.0f\%\n",
           root->name,
           parent_inst,
           root->stat->tog01_hit,
           miss01,
           root->stat->tog_total,
           percent01,
           root->stat->tog10_hit,
           miss10,
           root->stat->tog_total,
           percent10 );

  curr = root->child_head;
  while( curr != NULL ) {
    toggle_instance_summary( ofile, curr, root->name );
    curr = curr->next;
  }

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param head   Pointer to head of module list to parse.

 Iterates through the module list displaying the toggle coverage for
 each module.
*/
void toggle_module_summary( FILE* ofile, mod_link* head ) {

  float     total_tog = 0;  /* Total number of bits in module                        */
  int       tog01_hit = 0;  /* Number of bits that toggled from 0 to 1               */
  int       tog10_hit = 0;  /* Number of bits that toggled from 1 to 0               */
  mod_inst* curr;           /* Pointer to current child module instance of this node */
  float     percent01;      /* Percentage of bits that toggled from 0 to 1           */
  float     percent10;      /* Percentage of bits that toggled from 1 to 0           */
  float     miss01;         /* Number of bits that did not toggle from 0 to 1        */
  float     miss10;         /* Number of bits that did not toggle from 1 to 0        */

  toggle_get_stats( head->mod->exp_head, head->mod->sig_head, &total_tog, &tog01_hit, &tog10_hit );

  /* Calculate for toggle01 */
  percent01 = ((tog01_hit / total_tog) * 100);
  miss01    = (total_tog - tog01_hit);

  /* Calculate for toggle10 */
  percent10 = ((tog10_hit / total_tog) * 100);
  miss10    = (total_tog - tog10_hit);

  fprintf( ofile, "  %-20.20s    %-20.20s    %3d/%3.0f/%3.0f      %3.0f%%            %3d/%3.0f/%3.0f      %3.0f%%\n", 
           head->mod->name,
           head->mod->filename,
           tog01_hit,
           miss01,
           total_tog,
           percent01,
           tog10_hit,
           miss10,
           total_tog,
           percent10 );

  if( head->next != NULL ) {
    toggle_module_summary( ofile, head->next );
  }

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param root   Pointer to root of instance module tree to parse.

 Displays the verbose toggle coverage results to the specified output stream on
 an instance basis.  The verbose toggle coverage includes the signal names (and
 expressions) that did not receive 100% toggle coverage during simulation. 
*/
void toggle_instance_verbose( FILE* ofile, mod_inst* root ) {

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param head   Pointer to head of module list to parse.

 Displays the verbose toggle coverage results to the specified output stream on
 a module basis (combining modules that are instantiated multiple times).
 The verbose toggle coverage includes the signal names (and expressions) that
 did not receive 100% toggle coverage during simulation.
*/
void toggle_module_verbose( FILE* ofile, mod_link* head ) {

}

/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information
 \param instance  Specifies to report by instance or module.

 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the toggle coverage for each module encountered.  The parent module will
 specify its own toggle coverage along with a total toggle coverage including its 
 children.
*/
void toggle_report( FILE* ofile, bool verbose, bool instance ) {

  if( instance ) {

    fprintf( ofile, "TOGGLE COVERAGE RESULTS BY INSTANCE\n" );
    fprintf( ofile, "-----------------------------------\n" );
    fprintf( ofile, "Instance                  Parent                          Toggle 0 -> 1                    Toggle 1 -> 0\n" );
    fprintf( ofile, "                                                 Hit/Miss/Total    Percent hit    Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------\n" );

    toggle_instance_summary( ofile, instance_root, "<root>" );
    
    if( verbose ) {
      toggle_instance_verbose( ofile, instance_root );
    }

  } else {

    fprintf( ofile, "TOGGLE COVERAGE RESULTS BY MODULE\n" );
    fprintf( ofile, "---------------------------------\n" );
    fprintf( ofile, "Module                    Filename                        Toggle 0 -> 1                    Toggle 1 -> 0\n" );
    fprintf( ofile, "                                                 Hit/Miss/Total    Percent hit    Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------\n" );

    toggle_module_summary( ofile, mod_head );

    if( verbose ) {
      toggle_module_verbose( ofile, mod_head );
    }

  }

  fprintf( ofile, "===============================================================================================================\n" );
  fprintf( ofile, "\n" );

}
