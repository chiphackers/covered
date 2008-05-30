/*
 Name:        rshift1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/29/2008
 Purpose:     Verify right shift for ranges that are within the same 32-bit value but overshift.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a;

initial begin
	a = {5'b1xz01,27'h0};
	#5;
	a = a >> 29;
end

initial begin
`ifdef DUMP
        $dumpfile( "rshift1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
