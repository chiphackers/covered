/*
 Name:        cond3.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/24/2008
 Purpose:     Perform a conditional that contains a Z-value that does not
              achieve full coverage.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [2:0] a;
reg       b;

initial begin
	a = 0;
	b = 1'b1;
	#5;
	a = b ? 3'bxxx : 3'bzzz;
end

initial begin
`ifdef DUMP
        $dumpfile( "cond3.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
