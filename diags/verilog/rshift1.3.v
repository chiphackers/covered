/*
 Name:        rshift1.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/29/2008
 Purpose:     Verify right shift for ranges that are within the same 32-bit value but overshift.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a;

initial begin
	a = 32'b1xz01xz01xz01xz01xz01xz01xz01xz0;
	repeat( 32 ) begin
	  #5;
	  a = a >> 1;
	end
end

initial begin
`ifdef DUMP
        $dumpfile( "rshift1.3.vcd" );
        $dumpvars( 0, main );
`endif
        #500;
        $finish;
end

endmodule
