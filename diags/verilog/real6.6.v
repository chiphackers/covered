/*
 Name:        real6.6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies greater-than for real-binary operations.
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (4 > 3.9995);
	#5;
	a = (3.9995 > 4);
end

initial begin
`ifdef DUMP
        $dumpfile( "real6.6.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
