#include <tcl.h>
#include <tk.h>

int tcl_func_get_module_list( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] );

void tcl_func_initialize( Tcl_Interp* tcl, char* home );
