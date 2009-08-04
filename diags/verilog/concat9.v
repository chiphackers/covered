/*
 Name:        concat9.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/04/2009
 Purpose:     Verify that a concatenation with a sized decimal value is handled properly.
*/

module main;

wire [11:0] a;
reg         b;

assign a = b ? {1'b0,10'd44,1'b1} : {1'b1,10'd42,1'b1};

initial begin
`ifdef DUMP
        $dumpfile( "concat9.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        #10;
        $finish;
end

endmodule
