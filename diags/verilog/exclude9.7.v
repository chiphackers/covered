/*
 Name:        exclude9.7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/22/2008
 Purpose:     Verify that detailed line coverage is excluded properly via the coverage off/on
              pragma.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg a, b;

// coverage off
initial begin
	a = 1'b0;
	if( a )
          b = 1'b1;
end
// coverage on

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
