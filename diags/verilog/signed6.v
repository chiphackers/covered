/*
 Name:        signed6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/09/2009
 Purpose:     Verifies that signed outputs work properly.
*/

module main;

wire signed [4:0] x;

foo f( x );

initial begin
`ifdef DUMP
        $dumpfile( "signed6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


module foo(c);

output signed [4:0] c;

reg [3:0] a;
reg [3:0] b;
reg signed [4:0] c;

initial begin
	a = 4'h3;
	b = 4'h4;
	#5;
	c = a - b;
end

endmodule
