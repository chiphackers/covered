/*
 Name:        ifndef1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/03/2009
 Purpose:     Verifies that the `ifndef preprocessor directive works properly.
*/

module main;

`ifndef FOOBAR
reg a;
initial begin
	a = 1'b0;
	#5;
	a = 1'b1;
end
`endif

`ifndef RUNTEST
reg b;
initial begin
	b = 1'b0;
	#5;
	b = 1'b1;
end
`endif

initial begin
`ifdef DUMP
        $dumpfile( "ifndef1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
