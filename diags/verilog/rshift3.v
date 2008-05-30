/*
 Name:        rshift3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/29/2008
 Purpose:     Test fourth type of right shift condition.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [63:0] a;

initial begin
	a = 64'b1xz01xz01xz01xz01xz01xz01xz01xz0_1xz01xz01xz01xz01xz01xz01xz01xz0;
	#5;
	a = a >> 1;
end

initial begin
`ifdef DUMP
        $dumpfile( "rshift3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
