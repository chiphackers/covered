/*
 Name:        merge4a.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        06/18/2008
 Purpose:     Used to verify that files can be merged and output even when the -o
              option is not specified.
*/

module main;

reg  a, b;
wire c, d;

adder1 addA( a, b, c, d );

initial begin
	a = 1'b0;
	b = 1'b0;
	#5;
	a = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "merge4a.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
