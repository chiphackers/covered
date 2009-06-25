/*
 Name:        generate18.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/20/2009
 Purpose:     
*/

module main;

reg [1:0]  b;
reg [31:0] a;

initial begin
	b = 2'h0;
        #5;
        b = a[32-8-1-2-1:32-8-1-2-2];
end

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
	a = 32'h00080000;
        #10;
        $finish;
end

endmodule
