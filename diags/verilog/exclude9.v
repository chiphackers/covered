/*
 Name:        exclude9.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verifies that we can change the coverage keyword to something of our
              own choosing and things still work properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

// covered_coverage off
reg a;
// covered_coverage on

initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
