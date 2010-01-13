/*
 Name:        clog2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        01/12/2010
 Purpose:     Verifies clog2 system operation.
*/

module main;

integer a;

initial begin
	a = 0;
	#5;
	a = $clog2( 33 );
end

initial begin
`ifdef DUMP
        $dumpfile( "clog2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
