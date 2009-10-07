#include <verilated.h>             // Defines common routines
#include "Valways9.h"                 // From Verilating "aedge1.1.v"
#include <SpTraceVcdC.h>           // Trace file format header (from SystemPerl)

Valways9 *top;                        // Instantiation of module

unsigned int main_time = 0;        // Current simulation time

double sc_time_stamp () {          // Called by $time in Verilog
  return main_time;
}

int main() {

  top = new Valways9;                 // Create instance

  Verilated::traceEverOn( true );  // Verilator must compute traced signals
  SpTraceVcdCFile* tfp = new SpTraceVcdCFile;
  top->trace( tfp, 99 );           // Trace 99 levels of hierarchy
  tfp->open( "always9.vcd" );         // Open the dump file

  top->gend_clock = 0;

  while( !Verilated::gotFinish() ) {
    top->gend_clock = (main_time % 2);   // Toggle clock
    top->eval();                   // Evaluate model
    tfp->dump( main_time );        // Create waveform trace for this timestamp
    cout << "Time: " << dec << main_time << endl;
    main_time++;                   // Time passes...
  }

  top->final();                    // Done simulating

  tfp->close();

}

