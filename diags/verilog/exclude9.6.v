/*
 Name:        exclude9.6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/01/2008
 Purpose:     Verifies that coverages can be embedded without error.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

// coverage off
reg a;

// coverage off
initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end
// coverage on

reg b;

// coverage on

initial begin
	b = 1'b1;
	#20;
	b = ~a;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude9.6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
