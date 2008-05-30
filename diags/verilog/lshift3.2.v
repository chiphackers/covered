/*
 Name:        lshift3.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        04/28/2008
 Purpose:     Verifies that left-shift works for non-symmetrical shifting.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [79:0] a;

initial begin
	a = 80'b1xz01xz01xz01xz0_1xz01xz01xz01xz01xz01xz01xz01xz0_1xz01xz01xz01xz01xz01xz01xz01xz0;
	repeat( 80 ) begin
  	  #5;
	  a = a << 1;
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "lshift3.2.vcd" );
        $dumpvars( 0, main );
`endif
        #500;
        $finish;
end

endmodule
