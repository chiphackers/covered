/*
 Name:        rshift2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/29/2008
 Purpose:     Verify that right-shift operator works correctly when
              shift value is divisable by 32.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [79:0] a;

initial begin
	a = {5'b1xz01,75'h0};
	#5;
	a = a >> 0;
end

initial begin
`ifdef DUMP
        $dumpfile( "rshift2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
