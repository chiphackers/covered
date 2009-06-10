/*
 Name:        lshift3.3.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2008
 Purpose:     Verifies that left-shift works for non-symmetrical shifting.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [64:0] a;

initial begin
	a = 65'b0_1xz01xz01xz01xz01xz01xz01xz01xz0_1xz01xz01xz01xz01xz01xz01xz01xz0;
	repeat( 65 ) begin
  	  #5;
	  a = a << 1;
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "lshift3.3.vcd" );
        $dumpvars( 0, main );
`endif
        #500;
        $finish;
end

endmodule
