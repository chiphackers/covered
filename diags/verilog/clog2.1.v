/*
 Name:        clog2.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/12/2010
 Purpose:     Verifies clog2 system operation with X input.
*/

module main;

integer a;

initial begin
	a = 0;
	#5;
	a = $clog2( 32'hx );
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
