/*
 Name:        rshift3.1.v
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
	repeat( 64 ) begin
	  #5;
	  a = a >> 1;
	end
	#5;
	$finish;
end

initial begin
`ifdef DUMP
        $dumpfile( "rshift3.1.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
