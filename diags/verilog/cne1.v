/*
 Name:        cne1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/11/2008
 Purpose:     Verifies that a CNE operation (!==) that is not fully hit outputs to the report file correctly.
*/

module main;

reg a, b, c;

initial begin
	a = 1'b0;
	b = 1'bx;
	c = 1'b0;
	#5;
	a = b !== c;
end

initial begin
`ifdef DUMP
        $dumpfile( "cne1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
