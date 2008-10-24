/*
 Name:        real5.10.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        10/23/2008
 Purpose:     Verifies unary +/- for real operations
*/

module main;

reg a;

initial begin
	a = 1'b0;
	#5;
	a = (+3.456 != -3.456);
end

initial begin
`ifdef DUMP
        $dumpfile( "real5.10.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
