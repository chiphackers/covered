#include "vpi_user.h"

int last_time = -1;

PLI_INT32 covered_value_change( struct t_cb_data* cb ) {

  s_vpi_value value;
  s_vpi_time  time;

  time.type = vpiSimTime;
  vpi_get_time( cb->obj, &time );

  if( time.low != last_time ) {
    vpi_printf( "Performing timestep\n" );
    last_time = time.low;
  }

  value.format = vpiBinStrVal;
  vpi_get_value( cb->obj, &value );

  vpi_printf( "In covered_value_change, name: %s, time: %d, value: %s\n",
              vpi_get_str( vpiName, cb->obj ), time.low, value.value.misc );

  return( 0 );

}

static int covered_sim_calltf( char* name ) {

  vpiHandle systf_handle, arg_iterator, module_handle;
  vpiHandle net_handle, net_iterator;

  vpi_printf( "In covered_sim_calltf, name: %s\n", name );

  systf_handle = vpi_handle( vpiSysTfCall, NULL );
  arg_iterator = vpi_iterate( vpiArgument, systf_handle );

  while( module_handle = vpi_scan( arg_iterator ) ) {

    vpi_printf( "Found module: %s\n", vpi_get_str( vpiName, module_handle ) );

    if( net_iterator = vpi_iterate( vpiNet, module_handle ) ) {

      while( net_handle = vpi_scan( net_iterator ) ) {

        vpi_printf( "  Found net: %s\n", vpi_get_str( vpiName, net_handle ) );

        // Add a callback for a value change to this net
        s_cb_data cb_func;
        cb_func.reason = cbValueChange;
        cb_func.cb_rtn = covered_value_change;
        cb_func.obj    = net_handle;
        vpi_register_cb( &cb_func );

      }

    }

    if( net_iterator = vpi_iterate( vpiReg, module_handle ) ) {

      while( net_handle = vpi_scan( net_iterator ) ) {

        vpi_printf( "  Found reg: %s\n", vpi_get_str( vpiName, net_handle ) );
 
        // Add a callback for a value change to this net
        s_cb_data cb_func;
        cb_func.reason = cbValueChange;
        cb_func.cb_rtn = covered_value_change;
        cb_func.obj    = net_handle;
        vpi_register_cb( &cb_func );

      }

    }

  }

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

void (*vlog_startup_routines[])() = {
	covered_register,
	0
};

