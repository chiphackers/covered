/*
 Name:        concat7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/27/2008
 Purpose:     Verify that sign-extension works properly when assigning to a concatenation.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [39:0] a, b;
integer    i;

initial begin
        i = 32'h80000000;
        $display( $time, "  i: %d", i );
	a = 40'h0;
	b = 40'h0;
	#5;
        {b,a} = i;
end

initial begin
`ifdef DUMP
        $dumpfile( "concat7.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
