/*!
 \file    vpi.c
 \author  Trevor Williams  (trevorw@charter.net)
 \date    5/6/2005
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vpi_user.h"
#ifdef CVER
#include "cv_vpi_user.h"
#endif
#include "defines.h"

/* If this value is set, no expression callbacks are enabled */
#define NO_EXPR_CALLBACKS 1

char      in_db_name[1024];
char      out_db_name[1024];
int       last_time           = -1;
mod_inst* vpi_curr_mod        = NULL;

char*     merge_in0           = NULL;
char*     merge_in1           = NULL;
bool      report_gui          = FALSE;
mod_inst* instance_root       = NULL;
mod_link* mod_head            = NULL;
mod_link* mod_tail            = NULL;
str_link* no_score_head       = NULL;
str_link* no_score_tail       = NULL;
bool      one_instance_found  = FALSE;
int       timestep_update     = 0;
bool      report_covered      = FALSE;
bool      flag_use_line_width = FALSE;
int       line_width          = DEFAULT_LINE_WIDTH;
bool      report_instance     = FALSE;
int       flag_race_check     = WARNING;

extern bool       flag_scored;
extern bool       debug_mode;
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;
extern char*      curr_inst_scope;
extern mod_inst*  curr_instance;
extern char*      user_msg[USER_MSG_LENGTH];


#ifdef VPI_DEBUG
void covered_expr_op_lookup( PLI_INT32 type, char* name ) {

  switch( type ) {
    case vpiMinusOp       :  strcpy( name, "Unary minus" );             break;
    case vpiPlusOp        :  strcpy( name, "Unary plus" );              break;
    case vpiNotOp         :  strcpy( name, "Unary not" );               break;
    case vpiBitNegOp      :  strcpy( name, "Bitwise negation" );        break;
    case vpiUnaryAndOp    :  strcpy( name, "Unary AND" );               break;
    case vpiUnaryNandOp   :  strcpy( name, "Unary NAND" );              break;
    case vpiUnaryOrOp     :  strcpy( name, "Unary OR" );                break;
    case vpiUnaryNorOp    :  strcpy( name, "Unary NOR" );               break;
    case vpiUnaryXNorOp   :  strcpy( name, "Unary XNOR" );              break;
    case vpiSubOp         :  strcpy( name, "Subtraction" );             break;
    case vpiDivOp         :  strcpy( name, "Division" );                break;
    case vpiModOp         :  strcpy( name, "Modulus" );                 break;
    case vpiEqOp          :  strcpy( name, "Equality" );                break;
    case vpiNeqOp         :  strcpy( name, "Inequality" );              break;
    case vpiCaseEqOp      :  strcpy( name, "Case equality" );           break;
    case vpiCaseNeqOp     :  strcpy( name, "Case inequality" );         break;
    case vpiGtOp          :  strcpy( name, "Greater than" );            break;
    case vpiGeOp          :  strcpy( name, "Greater than or equal" );   break;
    case vpiLtOp          :  strcpy( name, "Less than" );               break;
    case vpiLeOp          :  strcpy( name, "Less than or equal" );      break;
    case vpiLShiftOp      :  strcpy( name, "Left shift" );              break;
    case vpiRShiftOp      :  strcpy( name, "Right shift" );             break;
    case vpiAddOp         :  strcpy( name, "Addition" );                break;
    case vpiMultOp        :  strcpy( name, "Multiplication" );          break;
    case vpiLogAndOp      :  strcpy( name, "Logical AND" );             break;
    case vpiLogOrOp       :  strcpy( name, "Logical OR" );              break;
    case vpiBitAndOp      :  strcpy( name, "Bitwise AND" );             break;
    case vpiBitOrOp       :  strcpy( name, "Bitwise OR" );              break;
    case vpiBitXorOp      :  strcpy( name, "Bitwise XOR" );             break;
    case vpiBitXnorOp     :  strcpy( name, "Bitwise XNOR" );            break;
    case vpiConditionOp   :  strcpy( name, "Conditional" );             break;
    case vpiConcatOp      :  strcpy( name, "Concatenation" );           break;
    case vpiMultiConcatOp :  strcpy( name, "Multiple concatenation" );  break;
    case vpiEventOrOp     :  strcpy( name, "Event OR" );                break;
    case vpiNullOp        :  strcpy( name, "NULL" );                    break;
    case vpiListOp        :  strcpy( name, "List" );                    break;
    case vpiMinTypMaxOp   :  strcpy( name, "MinTypMax" );               break;
    case vpiPosedgeOp     :  strcpy( name, "Posedge" );                 break;
    case vpiNegedgeOp     :  strcpy( name, "Negedge" );                 break;
    case vpiArithLShiftOp :  strcpy( name, "Arithmetic left shift" );   break;
    case vpiArithRShiftOp :  strcpy( name, "Arithmetic right shift" );  break;
    case vpiPowerOp       :  strcpy( name, "Arithmetic power" );        break;
    default               :  strcpy( name, "UNKNOWN" );                 break;
  }

}

void covered_stmt_lookup( PLI_INT32 type, char* name ) {

  switch( type ) {
    case vpiBegin        :  strcpy( name, "Begin" );                 break;
    case vpiNamedBegin   :  strcpy( name, "Named begin" );           break;
    case vpiFork         :  strcpy( name, "Fork" );                  break;
    case vpiNamedFork    :  strcpy( name, "Named fork" );            break;
    case vpiIf           :  strcpy( name, "If" );                    break;
    case vpiIfElse       :  strcpy( name, "If/Else" );               break;
    case vpiWhile        :  strcpy( name, "While" );                 break;
    case vpiRepeat       :  strcpy( name, "Repeat" );                break;
    case vpiWait         :  strcpy( name, "Wait" );                  break;
    case vpiCase         :  strcpy( name, "Case" );                  break;
    case vpiCaseItem     :  strcpy( name, "Case item" );             break;
    case vpiFor          :  strcpy( name, "For" );                   break;
    case vpiDelayControl :  strcpy( name, "Delay control" );         break;
    case vpiEventControl :  strcpy( name, "Event control" );         break;
    case vpiEventStmt    :  strcpy( name, "Event statement" );       break;
    case vpiAssignment   :  strcpy( name, "Assignment" );            break;
    case vpiDisable      :  strcpy( name, "Disable" );               break;
    case vpiForever      :  strcpy( name, "Forever" );               break;
    case vpiForce        :  strcpy( name, "Force" );                 break;
    case vpiAssignStmt   :  strcpy( name, "Assign" );                break;
    case vpiRelease      :  strcpy( name, "Release" );               break;
    case vpiDeassign     :  strcpy( name, "Deassign" );              break;
    case vpiTaskCall     :  strcpy( name, "Task call" );             break;
    case vpiFuncCall     :  strcpy( name, "Function call" );         break;
    case vpiSysTaskCall  :  strcpy( name, "System task call" );      break;
    case vpiSysFuncCall  :  strcpy( name, "System function call" );  break;
    case vpiNullStmt     :  strcpy( name, "Null" );                  break;
    default              :  strcpy( name, "UNKNOWN" );               break;
  }

}
#endif

PLI_INT32 covered_expr_change( p_cb_data cb ) {

#ifdef VPI_DEBUG
  char op_name[50];

  covered_expr_op_lookup( vpi_get( vpiOpType, cb->obj ), op_name );

  vpi_printf( "In covered_expr_change, Op: %s, time: %d, value: %s\n",
              op_name, cb->time->low, cb->value->value.str );
#endif

  return( 0 );

}

PLI_INT32 covered_value_change( p_cb_data cb ) {

#ifdef VPI_DEBUG
  vpi_printf( "In covered_value_change, name: %s, time: %d, value: %s\n",
              vpi_get_str( vpiFullName, cb->obj ), cb->time->low, cb->value->value.str );
#endif

  if( cb->time->low != last_time ) {
    if( last_time >= 0 ) {
      db_do_timestep( last_time );
    }
    last_time = cb->time->low;
  }
  
  /* Set symbol value */
  db_set_symbol_string( cb->user_data, cb->value->value.str );

  return( 0 );

}

PLI_INT32 covered_stmt_change( p_cb_data cb ) {

#ifdef VPI_DEBUG
  char stmt_name[50];

  covered_stmt_lookup( vpi_get( vpiType, cb->obj ), stmt_name );
  vpi_printf( "In covered_stmt_change, Stmt: %s, time: %d, value: %s\n", stmt_name, cb->time->low, cb->value->value.str );
#endif

  return( 0 );

}

PLI_INT32 covered_end_of_sim( p_cb_data cb ) {

#ifdef VPI_DEBUG
  vpi_printf( "At end of simulation, writing CDD contents\n" );
#endif

  /* Indicate that this CDD contains scored information */
  flag_scored = TRUE;

  /* Write contents to database file */
  if( !db_write( out_db_name, FALSE ) ) {
    vpi_printf( "Unable to write database file" );
    exit( 1 );
  }

  /* Deallocate memory */
  symtable_dealloc( vcd_symtab );
  if( timestep_tab != NULL ) {
    free_safe( timestep_tab );
  }

  return( 0 );

}

PLI_INT32 covered_cb_error_handler( p_cb_data cb ) {

  struct t_vpi_error_info einfotab;
  struct t_vpi_error_info *einfop;
  char   s1[128];

  einfop = &einfotab;
  vpi_chk_error( einfop );

  if( einfop->state == vpiCompile ) {
    strcpy( s1, "vpiCompile" );
  } else if( einfop->state == vpiPLI ) {
    strcpy( s1, "vpiPLI" );
  } else if( einfop->state == vpiRun ) {
    strcpy( s1, "vpiRun" );
  } else {
    strcpy( s1, "**unknown**" );
  }

  vpi_printf( "**ERR(%s) %s (level %d) at **%s(%d):\n  %s\n",
    einfop->code, s1, einfop->level, einfop->file, einfop->line, einfop->message );

  /* If serious error give up */
  if( (einfop->level == vpiError) || (einfop->level == vpiSystem) || (einfop->level == vpiInternal) ) {
    vpi_printf( "**FATAL: encountered error - giving up\n" );
    vpi_control( vpiFinish, 0 );
  }

  return( 0 );

}

char* gen_next_symbol() {

  static char symbol[21]   = {32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,'\0'};
  static int  symbol_index = 19;
  int         i            = 19;

  while( (i >= symbol_index) && (symbol[i] == 126) ) {
    symbol[i] = 33;
    if( (i - 1) < symbol_index ) {
      symbol_index--;
      if( symbol_index < 0 ) {
        return( NULL );
      }
    }
    i--;
  }
  symbol[i]++;

  return( strdup_safe( (symbol + symbol_index), __FILE__, __LINE__ ) );

}

void covered_create_value_change_cb( vpiHandle sig ) {

  p_cb_data cb;
  vsignal   vsig;
  sig_link* vsigl;
  char*     symbol;

  /* Find current signal in coverage database */
  vsig.name = vpi_get_str( vpiName, sig );
  if( (vsigl = sig_link_find( &vsig, vpi_curr_mod->mod->sig_head )) != NULL ) {

#ifdef VPI_DEBUG
    vpi_printf( "In covered_create_value_change_cb, adding callback for signal: %s\n", vsigl->sig->name );
#endif

    /* Generate new symbol */
    if( (symbol = gen_next_symbol()) == NULL ) {
      vpi_printf( "INTERNAL ERROR:  Unable to generate unique symbol name\n" );
      /* Need to increase number of characters in symbol array */
      vpi_control( vpiFinish, 0 );
    }

    /* Add signal/symbol to symtab database */
    db_assign_symbol( vsigl->sig->name, symbol, ((vsigl->sig->value->width + vsigl->sig->lsb) - 1), vsigl->sig->lsb ); 

    /* Add a callback for a value change to this net */
    cb                   = (p_cb_data)malloc( sizeof( s_cb_data ) );
    cb->reason           = cbValueChange;
    cb->cb_rtn           = covered_value_change;
    cb->obj              = sig;
    cb->time             = (p_vpi_time)malloc( sizeof( s_vpi_time ) ); 
    cb->time->type       = vpiSimTime;
    cb->time->high       = 0;
    cb->time->low        = 0;
    cb->value            = (p_vpi_value)malloc( sizeof( s_vpi_value ) );
    cb->value->format    = vpiBinStrVal;
    cb->value->value.str = NULL;
    cb->user_data        = symbol;
    vpi_register_cb( cb );

  }

}

void covered_create_expr_change_cb( vpiHandle expr ) {

#ifndef NO_EXPR_CALLBACKS
  p_cb_data cb;

#ifdef VPI_DEBUG
  vpi_printf( "In covered_create_expr_change_cb\n" );
#endif

  /* Add a callback for a value change to this expr */
  cb                   = (p_cb_data)malloc( sizeof( s_cb_data ) );
  cb->reason           = cbValueChange;
  cb->cb_rtn           = covered_expr_change;
  cb->obj              = expr;
  cb->time             = (p_vpi_time)malloc( sizeof( s_vpi_time ) );
  cb->time->type       = vpiSimTime;
  cb->time->high       = 0;
  cb->time->low        = 0;
  cb->value            = (p_vpi_value)malloc( sizeof( s_vpi_value ) );
  cb->value->format    = vpiBinStrVal;
  cb->value->value.str = NULL;
  vpi_register_cb( cb );
#endif

}

void covered_parse_expr( vpiHandle expr, int lhs ) {

  vpiHandle iter, handle;
  PLI_INT32 type = vpi_get( vpiType, expr );
#ifdef VPI_DEBUG
  char      op_name[50];
#endif

  /* We only need to get right-hand-side expression information for coverage */
  if( lhs == 0 ) {

    /* If this is an operation, parse sub-expressions and create a callback for the expression */
    if( type == vpiOperation ) {

      /* Parse sub-expressions */
      if( (iter = vpi_iterate( vpiOperand, expr )) != NULL ) {

        while( (handle = vpi_scan( iter )) != NULL ) {
          covered_parse_expr( handle, lhs );
        }

      }

#ifdef VPI_DEBUG
      covered_expr_op_lookup( vpi_get( vpiOpType, expr ), op_name );
      vpi_printf( "      EXPRESSION: %s\n", op_name );
#endif

#ifndef NO_EXPR_CALLBACKS
      covered_create_expr_change_cb( expr );
#endif

    } else if( type == vpiPartSelect ) {

      /* Parse left range */
      if( (handle = vpi_handle( vpiLeftRange, expr )) != NULL ) {
        covered_parse_expr( handle, 0 );
      }

      /* Parse right range */
      if( (handle = vpi_handle( vpiRightRange, expr )) != NULL ) {
        covered_parse_expr( handle, 0 );
      }

#ifdef VPI_DEBUG
      vpi_printf( "      PART_SELECT\n" );
#endif

#ifndef NO_EXPR_CALLBACKS
      covered_create_expr_change_cb( expr );
#endif

    } else if( type == vpiNet ) {

#ifdef VPI_DEBUG
      vpi_printf( "      NET\n" );
#endif

    } else if( type == vpiReg ) {

#ifdef VPI_DEBUG
      vpi_printf( "      REG\n" );
#endif

    } else if( type == vpiFuncCall ) {

#ifdef VPI_DEBUG
      vpi_printf( "      FUNCTION_CALL\n" );
#endif

#ifndef NO_EXPR_CALLBACKS
      covered_create_expr_change_cb( expr );
#endif

    } else if( type == vpiSysFuncCall ) {
 
#ifdef VPI_DEBUG
      vpi_printf( "      SYSTEM_FUNCTION_CALL\n" );
#endif

#ifndef NO_EXPR_CALLBACKS                                                                                                                  
      covered_create_expr_change_cb( expr );                                                                                          
#endif

    } else {

      /* ERROR */

    }

  } 

}

void covered_parse_cont_assigns( vpiHandle mod ) {

  vpiHandle iter, handle;

  if( (iter = vpi_iterate( vpiContAssign, mod )) != NULL ) {

    while( (handle = vpi_scan( iter )) != NULL ) {

#ifdef VPI_DEBUG
      vpi_printf( "  Found continuous assignment, file: %s, line: %d\n",
         vpi_get_str( vpiFile, handle ), vpi_get( vpiLineNo, handle ) );
#endif

      covered_parse_expr( vpi_handle( vpiLhs, handle ), 1 );
      covered_parse_expr( vpi_handle( vpiRhs, handle ), 0 );

    }

  }

}

void covered_parse_statement( vpiHandle stmt );

void covered_parse_block_statement( vpiHandle stmt ) {

  vpiHandle iter, handle;
#ifdef VPI_DEBUG
  char      type[20];

  switch( vpi_get( vpiType, stmt ) ) {
    case vpiBegin      :  strcpy( type, "Begin" );        break;
    case vpiNamedBegin :  strcpy( type, "Named begin" );  break;
    case vpiFork       :  strcpy( type, "Fork" );         break;
    case vpiNamedFork  :  strcpy( type, "Named fork" );   break;
    default            :  strcpy( type, "UNKNOWN" );      break;
  } 
  vpi_printf( "    %s, file: %s, line: %d\n",
    type, vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  if( (iter = vpi_iterate( vpiStmt, stmt )) != NULL ) {
  
    while( (handle = vpi_scan( iter )) != NULL ) {
      covered_parse_statement( handle );
    }

  }

}

void covered_parse_event_control( vpiHandle stmt ) {

  vpiHandle handle;

#ifdef VPI_DEBUG
  vpi_printf( "    Event control, file: %s, line: %d\n",
    vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse condition of this event control */
  if( (handle = vpi_handle( vpiCondition, stmt )) != NULL ) {
    covered_parse_expr( handle, 0 );
  }

  /* Parse following statement */
  if( (handle = vpi_handle( vpiStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

}

void covered_parse_assignment( vpiHandle stmt ) {

  vpiHandle handle;

#ifdef VPI_DEBUG
  vpi_printf( "    Assignment, file: %s, line: %d\n",
    vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse left-hand sides */
  if( (handle = vpi_handle( vpiLhs, stmt )) != NULL ) {
    covered_parse_expr( handle, 1 );
  }

  /* Parse the right-hand side */
  if( (handle = vpi_handle( vpiRhs, stmt )) != NULL ) {
    covered_parse_expr( handle, 0 );
  }
}

void covered_parse_if( vpiHandle stmt ) {

  vpiHandle handle;
  PLI_INT32 type = vpi_get( vpiType, stmt );

#ifdef VPI_DEBUG
  if( type == vpiIf) {
    vpi_printf( "    If, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  } else {
    vpi_printf( "    IfElse, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  }
#endif

  /* Parse conditional expression */
  if( (handle = vpi_handle( vpiCondition, stmt )) != NULL ) {
    covered_parse_expr( handle, 0 );
  }

  /* Parse true statement */
  if( (handle = vpi_handle( vpiStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

  /* If the current statement is an IfElse, parse false statement */
  if( (type == vpiIfElse) && ((handle = vpi_handle( vpiElseStmt, stmt )) != NULL) ) {
    covered_parse_statement( handle );
  }

}

void covered_parse_forever( vpiHandle stmt ) {

  vpiHandle handle;

#ifdef VPI_DEBUG
  vpi_printf( "    Forever, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse following statement */
  if( (handle = vpi_handle( vpiStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

}

void covered_parse_delay_control( vpiHandle stmt ) {

  vpiHandle iter, handle;

#ifdef VPI_DEBUG
  vpi_printf( "    Delay control, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse delay expression(s) */
  if( (iter = vpi_iterate( vpiDelay, stmt )) != NULL ) {
    while( (handle = vpi_scan( iter )) != NULL ) {
      covered_parse_expr( handle, 0 );
    }
  }

  /* Parse following statement */
  if( (handle = vpi_handle( vpiStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

}

void covered_parse_while_repeat_wait( vpiHandle stmt ) {

  vpiHandle handle;
#ifdef VPI_DEBUG
  PLI_INT32 type = vpi_get( vpiType, stmt );
  char      t[20];

  switch( type ) {
    case vpiWhile  :  strcpy( t, "While" );    break;
    case vpiRepeat :  strcpy( t, "Repeat" );   break;
    case vpiWait   :  strcpy( t, "Wait" );     break;
    default        :  strcpy( t, "UNKNOWN" );  break;
  }
  vpi_printf( "    %s, file: %s, line: %d\n", t, vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse conditional expression */
  if( (handle = vpi_handle( vpiCondition, stmt )) != NULL ) {
    covered_parse_expr( handle, 0 );
  }

  /* Parse following statement */
  if( (handle = vpi_handle( vpiStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

}

void covered_parse_case( vpiHandle stmt ) {

  vpiHandle handle;
#ifdef VPI_DEBUG
  PLI_INT32 type = vpi_get( vpiCaseType, stmt );
  char      t[20];

  switch( type ) {
    case vpiCaseExact :  strcpy( t, "Case" );    break;
    case vpiCaseX     :  strcpy( t, "Casex" );   break;
    case vpiCaseZ     :  strcpy( t, "Casez" );   break;
    default           :  strcpy( t, "UKNOWN" );  break;
  }
  vpi_printf( "    %s, file: %s, line: %d\n", t, vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse case condition */
  if( (handle = vpi_handle( vpiCondition, stmt )) != NULL ) {
    covered_parse_expr( handle, 0 );
  }

}

void covered_parse_case_item( vpiHandle stmt ) {

  vpiHandle iter, handle;

#ifdef VPI_DEBUG
  vpi_printf( "    Case item, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse case expression(s) */
  if( (iter = vpi_iterate( vpiExpr, stmt )) != NULL ) {
    while( (handle = vpi_scan( iter )) != NULL ) {
      covered_parse_expr( handle, 0 );
    }
  }

  /* Parse following statement */
  if( (handle = vpi_handle( vpiStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

}

void covered_parse_for( vpiHandle stmt ) {

  vpiHandle handle;

#ifdef VPI_DEBUG
  vpi_printf( "    For, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
#endif

  /* Parse initialization statement */
  if( (handle = vpi_handle( vpiForInitStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

  /* Parse conditional expression */
  if( (handle = vpi_handle( vpiCondition, stmt )) != NULL ) {
    covered_parse_expr( handle, 0 );
  }

  /* Parse increment statement */
  if( (handle = vpi_handle( vpiForIncStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

  /* Parse following statement */
  if( (handle = vpi_handle( vpiStmt, stmt )) != NULL ) {
    covered_parse_statement( handle );
  }

}

void covered_parse_force_proc_assign( vpiHandle stmt ) {

  vpiHandle handle;
#ifdef VPI_DEBUG
  PLI_INT32 type = vpi_get( vpiType, stmt );

  if( type == vpiForce ) {
    vpi_printf( "    Force, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  } else {
    vpi_printf( "    Assign, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  }
#endif

  /* Parse right-hand-side expression */
  if( (handle = vpi_handle( vpiRhs, stmt )) != NULL ) {
    covered_parse_expr( handle, 0 );
  }

  /* Parse left-hand-side expression */
  if( (handle = vpi_handle( vpiLhs, stmt )) != NULL ) {
    covered_parse_expr( handle, 1 );
  }

}

void covered_parse_release_deassign( vpiHandle stmt ) {

  vpiHandle handle;
#ifdef VPI_DEBUG
  PLI_INT32 type = vpi_get( vpiType, stmt );

  if( type == vpiRelease ) {
    vpi_printf( "    Release, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  } else {
    vpi_printf( "    Deassign, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  }
#endif

  /* Parse left-hand-side expression */
  if( (handle = vpi_handle( vpiLhs, stmt )) != NULL ) {
    covered_parse_expr( handle, 1 );
  }

}

void covered_parse_task_func( vpiHandle mod ) {

  vpiHandle iter, handle;
  vpiHandle liter, scope;
  mod_inst* tmp_mod;
	
  /* Parse all internal scopes for tasks and functions */
  if( (iter = vpi_iterate( vpiInternalScope, mod )) != NULL ) {

    while( (scope = vpi_scan( iter )) != NULL ) {
      
#ifdef VPI_DEBUG
      vpi_printf( "Parsing task/function %s\n", vpi_get_str( vpiFullName, scope ) );
#endif

      tmp_mod = vpi_curr_mod;

      if( (vpi_curr_mod = instance_find_scope( instance_root, vpi_get_str( vpiFullName, scope ) )) != NULL ) {

        /* Set current scope in database */
        if( curr_inst_scope != NULL ) {
          free_safe( curr_inst_scope, __FILE__, __LINE__ );
        }
        curr_inst_scope = strdup_safe( vpi_get_str( vpiFullName, scope ), __FILE__, __LINE__ );

        /* Parse signals */
        if( (liter = vpi_iterate( vpiReg, scope )) != NULL ) {
          while( (handle = vpi_scan( liter )) != NULL ) {
#ifdef VPI_DEBUG
            vpi_printf( "  Found reg %s, file: %s, line: %d\n",
              vpi_get_str( vpiFullName, handle ), vpi_get_str( vpiFile, handle ), vpi_get( vpiLineNo, handle ) );
#endif
            covered_create_value_change_cb( handle );
          }
        }

        /* Parse the associated statement */
        if( (handle = vpi_handle( vpiStmt, scope )) != NULL ) {
          covered_parse_statement( handle );
        }

        /* Recursively check scope */
        if( (liter = vpi_iterate( vpiInternalScope, scope )) != NULL ) {
          while( (handle = vpi_scan( liter )) != NULL ) {
            covered_parse_task_func( handle );
          }
        }

      }

      vpi_curr_mod = tmp_mod;
 
    }

  }

}

void covered_parse_call( vpiHandle stmt ) {

  vpiHandle iter, handle;
#ifdef VPI_DEBUG
  PLI_INT32 type = vpi_get( vpiType, stmt );

  if( type == vpiTaskCall ) {
    vpi_printf( "    Task call, name: %s, file: %s, line: %d\n",
      vpi_get_str( vpiName, stmt ), vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  } else if( type == vpiFuncCall ) {
    vpi_printf( "    Function call, name: %s, file: %s, line: %d\n",
      vpi_get_str( vpiName, stmt ), vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  }
#endif

  /* Parse expression list */
  if( (iter = vpi_iterate( vpiExpr, stmt )) != NULL ) {
    while( (handle = vpi_scan( iter )) != NULL ) {
      covered_parse_expr( handle, 0 );
    }
  }

}

void covered_parse_event_disable( vpiHandle stmt ) {

#ifdef VPI_DEBUG
  PLI_INT32 type = vpi_get( vpiType, stmt );

  if( type == vpiEventStmt ) {
    vpi_printf( "    Event statement, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  } else {
    vpi_printf( "    Disable, file: %s, line: %d\n", vpi_get_str( vpiFile, stmt ), vpi_get( vpiLineNo, stmt ) );
  }
#endif

}

void covered_parse_statement( vpiHandle stmt ) {

  PLI_INT32 type = vpi_get( vpiType, stmt );
#ifdef VPI_DEBUG
  char      t[50];
#endif

  /* Check block assignments */
  switch( type ) {
    case vpiBegin        :
    case vpiNamedBegin   :
    case vpiFork         :
    case vpiNamedFork    :  covered_parse_block_statement( stmt );       break;
    case vpiIf           :
    case vpiIfElse       :  covered_parse_if( stmt );                    break;
    case vpiWhile        :
    case vpiRepeat       :
    case vpiWait         :  covered_parse_while_repeat_wait( stmt );     break;
    case vpiCase         :  covered_parse_case( stmt );                  break;
    case vpiCaseItem     :  covered_parse_case_item( stmt );             break;
    case vpiFor          :  covered_parse_for( stmt );                   break;
    case vpiDelayControl :  covered_parse_delay_control( stmt );         break;
    case vpiEventControl :  covered_parse_event_control( stmt );         break;
    case vpiAssignment   :  covered_parse_assignment( stmt );            break;
    case vpiDisable      :
    case vpiEventStmt    :  covered_parse_event_disable( stmt );         break;
    case vpiForever      :  covered_parse_forever( stmt );               break;
    case vpiForce        :
    case vpiAssignStmt   :  covered_parse_force_proc_assign( stmt );     break;
    case vpiRelease      :
    case vpiDeassign     :  covered_parse_release_deassign( stmt );      break;
    case vpiTaskCall     :
    case vpiFuncCall     :  covered_parse_call( stmt );                  break;
    case vpiSysTaskCall  :
    case vpiSysFuncCall  :
    case vpiNullStmt     :
    default              :
#ifdef VPI_DEBUG
      covered_stmt_lookup( vpi_get( vpiType, stmt ), t );
      vpi_printf( "    %s\n", t );
#endif
      break;
  }
 
}

void covered_parse_processes( vpiHandle mod ) {

  vpiHandle iter, handle;
  PLI_INT32 type;

  if( (iter = vpi_iterate( vpiProcess, mod )) != NULL ) {

    while( (handle = vpi_scan( iter )) != NULL ) {

      type = vpi_get( vpiType, handle );

      /* Parse always blocks */ 
      if( type == vpiAlways ) {
#ifdef VPI_DEBUG
        vpi_printf( "  Found always block\n" );
#endif
        covered_parse_statement( vpi_handle( vpiStmt, handle ) );

      /* Parse initial blocks */
      } else if( type == vpiInitial ) {
#ifdef VPI_DEBUG
        vpi_printf( "  Found initial block\n" );
#endif
        covered_parse_statement( vpi_handle( vpiStmt, handle ) );
      }

    }

  }

}

void covered_parse_signals( vpiHandle mod ) {

  vpiHandle iter, handle;

  /* Parse nets */
  if( (iter = vpi_iterate( vpiNet, mod )) != NULL ) {

    while( (handle = vpi_scan( iter )) != NULL ) {

#ifdef VPI_DEBUG
      vpi_printf( "  Found net: %s\n", vpi_get_str( vpiName, handle ) );
#endif

      covered_create_value_change_cb( handle );

    }

  }

  /* Parse regs */
  if( (iter = vpi_iterate( vpiReg, mod )) != NULL ) {

    while( (handle = vpi_scan( iter )) != NULL ) {

#ifdef VPI_DEBUG
      vpi_printf( "  Found reg: %s\n", vpi_get_str( vpiName, handle ) );
#endif

      covered_create_value_change_cb( handle );

    }

  }

}

void covered_parse_instance( vpiHandle inst ) {

  vpiHandle  iter, handle;
  PLI_BYTE8* mod_path = vpi_get_str( vpiFullName, inst );

  /* Find current module in stored design */
  if( (vpi_curr_mod = instance_find_scope( instance_root, (char*)mod_path )) != NULL ) {

#ifdef VPI_DEBUG
    vpi_printf( "Found module to be covered: %s, full name: %s\n", vpi_get_str( vpiName, inst ), mod_path );
#endif

    /* Set database curr_instance */
    curr_instance      = vpi_curr_mod;
    one_instance_found = TRUE;

    /* Set current scope in database */
    if( curr_inst_scope != NULL ) {
      free_safe( curr_inst_scope, __FILE__, __LINE__ );
    }
    curr_inst_scope = strdup_safe( vpi_get_str( vpiFullName, inst ), __FILE__, __LINE__ );

    /* Parse all signals */
    covered_parse_signals( inst );

    /* Parse all continual assignments */
    covered_parse_cont_assigns( inst );

    /* Parse all processes */
    covered_parse_processes( inst );

    /* Parse all functions/tasks */
    covered_parse_task_func( inst );

  }

  if( (iter = vpi_iterate( vpiModule, inst )) != NULL ) {

    while( (handle = vpi_scan( iter )) != NULL ) {
      covered_parse_instance( handle );
    }

  }

}

PLI_INT32 covered_sim_calltf( PLI_BYTE8* name ) {

  vpiHandle       systf_handle, arg_iterator, module_handle;
  vpiHandle       arg_handle;
  s_vpi_vlog_info info;
  p_cb_data       cb;
  int             i;
  char*           argvptr;

#ifdef VPI_DEBUG
  vpi_printf( "In covered_sim_calltf, name: %s\n", name );
  debug_mode = TRUE;
#endif

  systf_handle = vpi_handle( vpiSysTfCall, NULL );
  arg_iterator = vpi_iterate( vpiArgument, systf_handle );

  /* Create callback that will handle the end of simulation */
  cb            = (p_cb_data)malloc( sizeof( s_cb_data ) );
  cb->reason    = cbEndOfSimulation;
  cb->cb_rtn    = covered_end_of_sim;
  cb->obj       = NULL;
  cb->time      = NULL;
  cb->value     = NULL;
  cb->user_data = NULL;
  vpi_register_cb( cb );

  /* Create error handling callback */
  cb            = (p_cb_data)malloc( sizeof( s_cb_data ) );
  cb->reason    = cbPLIError;
  cb->cb_rtn    = covered_cb_error_handler;
  cb->obj       = NULL;
  cb->time      = NULL;
  cb->value     = NULL;
  cb->user_data = NULL;
  vpi_register_cb( cb );

  /* Get name of CDD database file from system call arguments */
  if( (arg_handle = vpi_scan( arg_iterator )) != NULL ) {
    s_vpi_value data;
    data.format = vpiStringVal;
    vpi_get_value( arg_handle, &data );
    strcpy( in_db_name, data.value.str );
  }

  /* Get name of CDD database to write to (default is cov.cdd) */
  strcpy( out_db_name, "cov.cdd" );
  if( vpi_get_vlog_info( &info ) ) {
    for( i=1; i<info.argc; i++ ) {
      argvptr = info.argv[i];
      if( strncmp( "+covered_outdb=", argvptr, 15 ) == 0 ) {
        argvptr += 15;
        strcpy( out_db_name, argvptr );
      }
    }
  }

#ifdef VPI_DEBUG
  vpi_printf( "========  Reading in database %s  ========\n", in_db_name );
#endif

  /* Initialize all global information variables */
  info_initialize();
    
  /* Read in contents of specified database file */
  if( !db_read( in_db_name, READ_MODE_MERGE_NO_MERGE ) ) {
    vpi_printf( "Unable to read database file" );
    exit( 1 );
  }   

  /* Create initial symbol table */
  vcd_symtab = symtable_create();

  /* Parse child instances */
  while( (module_handle = vpi_scan( arg_iterator )) != NULL ) {
    covered_parse_instance( module_handle );
  }

  /* Create timestep symbol table array */
  if( vcd_symtab_size > 0 ) {
    timestep_tab = malloc_safe_nolimit( (sizeof( symtable*) * vcd_symtab_size), __FILE__, __LINE__ );
  }

#ifdef VPI_DEBUG
  vpi_printf( "\n*************************************************************\n\n" );
#endif

  return 0;

}

void covered_register() {

  s_vpi_systf_data tf_data;

  tf_data.type      = vpiSysTask;
  tf_data.tfname    = "$covered_sim";
  tf_data.calltf    = covered_sim_calltf;
  tf_data.compiletf = 0;
  tf_data.sizetf    = 0;
  tf_data.user_data = "$covered_sim";
  vpi_register_systf( &tf_data );

}

void vpi_generate_top_module( char* vpi_file, char* output_db, char* top_inst ) {

  FILE* vfile;  /* File handle to VPI top-level module */

  /* Create filename for VPI-based module */
  snprintf( user_msg, USER_MSG_LENGTH, "%s.v", vpi_file );

  if( (vfile = fopen( user_msg, "w" )) != NULL ) {

    fprintf( vfile, "module %s;\ninitial $covered_sim( \"%s\", %s );\nendmodule\n", vpi_file, output_db, top_inst );
    fclose( vfile );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Unable to open %s for writing", vpi_file );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );
 
  }

}

#ifdef CVER
void (*vlog_startup_routines[])() = {
	covered_register,
	0
};

void vpi_compat_bootstrap(void)
{
 int i;

 for (i = 0;; i++)
  {
   if (vlog_startup_routines[i] == NULL) break;
   vlog_startup_routines[i]();
  }
}
#endif

