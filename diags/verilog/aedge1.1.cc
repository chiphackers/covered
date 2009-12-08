#include <verilated.h>             // Defines common routines
#include "Vaedge1.h"                 // From Verilating 					//"aedge1.1.verilator.v"
#ifdef COVERED_INLINED
#include "Vaedge1_main.h"
#endif
#include <SpTraceVcdC.h>           // Trace file format header (from SystemPerl)

Vaedge1 *top;                        // Instantiation of module

unsigned int main_time = 0;        // Current simulation time

double sc_time_stamp () {          // Called by $time in Verilog
  return main_time;
}

int main() {

  top = new Vaedge1;                 // Create instance

  Verilated::traceEverOn( true );  // Verilator must compute traced signals
  SpTraceVcdCFile* tfp = new SpTraceVcdCFile;
  top->trace( tfp, 99 );           // Trace 99 levels of hierarchy
  tfp->open( "aedge1.1.vcd" );         // Open the dump file

#ifdef COVERED_INLINED
  covered_initialize( top, "../aedge1.1.cdd" );
#endif

  top->gend_clock = 0;

  while( !Verilated::gotFinish() ) {
    top->gend_clock = (main_time % 2);   // Toggle clock
    top->eval();                   // Evaluate model
    tfp->dump( main_time );        // Create waveform trace for this timestamp
    cout << "Time: " << dec << main_time << endl;
    main_time++;                   // Time passes...
  }

  top->final();                    // Done simulating

#ifdef COVERED_INLINED
  covered_close( "../aedge1.1.cdd" );
#endif

  tfp->close();

}

