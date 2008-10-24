/*
 Name:        real6.4.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verify less-than for real-binary operations
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (3.9999 < 4);
	#5;
	a = (4 < 3.9999);
end

initial begin
`ifdef DUMP
        $dumpfile( "real6.4.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
