#include <verilated.h>             // Defines common routines
#include "Vgenerate12.h"           // From Verilating "add1.v"
#ifdef COVERED_INLINED
#include "Vgenerate12_main.h"
#endif
#include <SpTraceVcdC.h>           // Trace file format header (from SystemPerl)

Vgenerate12 *top;                  // Instantiation of module

unsigned int main_time = 0;        // Current simulation time

double sc_time_stamp () {          // Called by $time in Verilog
  return main_time;
}

int main() {

  top = new Vgenerate12;           // Create instance

  Verilated::traceEverOn( true );  // Verilator must compute traced signals
  SpTraceVcdCFile* tfp = new SpTraceVcdCFile;
  top->trace( tfp, 99 );           // Trace 99 levels of hierarchy
  tfp->open( "generate12.vcd" );   // Open the dump file

#ifdef COVERED_INLINED
  covered_initialize( top, "../generate12.cdd" );
#endif

  top->verilatorclock = 0;

  while( !Verilated::gotFinish() ) {
    top->verilatorclock = (main_time % 2);   // Toggle clock
    top->eval();                   // Evaluate model
    tfp->dump( main_time );        // Create waveform trace for this timestamp
    main_time++;                   // Time passes...
  }

  top->final();                    // Done simulating

#ifdef COVERED_INLINED
  covered_close( "../generate12.cdd" );
#endif

  tfp->close();

}

