/*
 Name:        cond3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Perform a conditional that contains a decimal value that does not
              achieve full coverage.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a;
reg        b;

initial begin
	a = 0;
	b = 1'b1;
	#5;
	a = b ? 2 : 3;
end

initial begin
`ifdef DUMP
        $dumpfile( "cond3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
