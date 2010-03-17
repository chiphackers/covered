/*
 Name:        expand6.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        02/28/2010
 Purpose:     Verifies that a zero-width expression within a concatenation is legal and works properly.
*/

module main;

reg [31:0] a, d;
reg        b, c;

initial begin
	a = 32'h0;
        b = 1'b1;
        c = 1'b1;
	d = 32'h80000000;
	#5;
	a = {{0{b & c}}, {0{b | c}}, d};
end

initial begin
`ifdef DUMP
        $dumpfile( "expand6.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
